// Standard C++ libraries for input/output, strings, vectors, file operations, and system calls.
#include <iostream>
#include <string>
#include <vector>
#include <fstream> // Required for std::ifstream (file input)
#include <cstdlib> // Required for system() (executing shell commands)
#include <cassert> // Required for assert() (debugging checks)
#include <cstring> // Required for C-style string functions
#include <thread>  // For std::thread::hardware_concurrency()

// Main header file for the Cactus library.
#include "../../cactus/cactus.h"

// --- Helper function to check if a file exists ---
bool fileExists(const std::string& filepath) {
    std::ifstream f(filepath.c_str()); 
    return f.good(); 
}

bool downloadFile(const std::string& url, const std::string& filepath, const std::string& filename_desc) {
    if (url.empty() || filepath.empty()) { // Don't attempt download if no URL/path
        if (filepath.empty()) {
             std::cout << "No filepath specified for " << filename_desc << ", skipping download." << std::endl;
        } else {
            std::cout << "No URL specified for " << filename_desc << " at " << filepath << ", skipping download." << std::endl;
        }
        // If file still exists locally, consider it usable.
        return fileExists(filepath);
    }

    if (fileExists(filepath)) {
        std::cout << filename_desc << " already exists at " << filepath << std::endl;
        return true;
    }

    std::cout << "Downloading " << filename_desc << " from " << url << " to " << filepath << "..." << std::endl;

    std::string command = "curl -L -o \"" + filepath + "\" \"" + url + "\"";
    
    int return_code = system(command.c_str());

    if (return_code == 0 && fileExists(filepath)) {
        std::cout << filename_desc << " downloaded successfully." << std::endl;
        return true;
    } else {
        std::cerr << "Failed to download " << filename_desc << "." << std::endl;
        if (return_code != 0) {
            std::cerr << "Curl command exited with code: " << return_code << std::endl;
        }
        std::cerr << "Please ensure curl is installed and the URL is correct." << std::endl;
        std::cerr << "You can try downloading it manually using the command:" << std::endl;
        std::cerr << command << std::endl;
        
        if (fileExists(filepath)) {
            // If download failed but file was created (e.g. empty or partial), remove it.
            std::remove(filepath.c_str());
        }
        return false;
    }
}

int main(int argc, char **argv) {
    // Primary TTS Model (e.g., OuteTTS text-to-codes model)
    const std::string model_url = "https://huggingface.co/OuteAI/OuteTTS-0.3-500M-GGUF/resolve/main/OuteTTS-0.3-500M-Q6_K.gguf";
    const std::string model_filename = "OuteTTS-0.3-500M-Q6_K.gguf";

    // Vocoder Model (e.g., WavTokenizer codes-to-speech model)
    const std::string vocoder_model_url = "https://huggingface.co/ggml-org/WavTokenizer/resolve/main/WavTokenizer-Large-75-F16.gguf"; // Example - check for actual model
    const std::string vocoder_model_filename = "WavTokenizer-Large-75-F16.gguf"; 
    
    const std::string output_wav_filename = "output.wav";

    // Attempt to download models if they don't exist
    if (!downloadFile(model_url, model_filename, "Primary TTS Model")) {
        std::cerr << "Essential Primary TTS Model could not be downloaded or found. Exiting." << std::endl;
        return 1;
    }
    // Vocoder download is also treated as essential if a path is provided, as our current TTS logic expects it.
    if (!vocoder_model_filename.empty()) {
        if (!downloadFile(vocoder_model_url, vocoder_model_filename, "Vocoder Model")) {
            std::cerr << "Vocoder Model could not be downloaded or found. Exiting as it's required by current setup." << std::endl;
            return 1; 
        }
    }

    common_params params;
    params.model.path = model_filename;
    if (!vocoder_model_filename.empty()) {
        params.vocoder.model.path = vocoder_model_filename;
    }
    // Optional: path to a speaker embedding JSON file (if your model/setup uses one)
    // params.vocoder.speaker_file = "path/to/your/speaker.json"; 
    // params.vocoder.use_guide_tokens = false; // Set true if using guide tokens (often with speaker embeddings)

    params.prompt = "This is a test run of the text to speech system for Cactus, I hope you enjoy it as much as i do, thank you";
    
    // --- General Generation Control Parameters (some might be more relevant for primary TTS model) ---
    params.n_predict = 768; // Max number of "codes" to predict. Adjusted from 768.
    params.n_ctx = 2048;    // Context window for primary TTS model.
    params.n_batch = 512;   // Batch size for prompt processing.
    unsigned int n_threads = std::thread::hardware_concurrency();
    params.cpuparams.n_threads = n_threads > 0 ? n_threads : 4; // Default to 4 if hardware_concurrency is 0
    params.use_mmap = true;
    params.warmup = false; // Warmup usually not critical for single TTS generation

    // --- Sampling Parameters (for primary TTS model) ---
    params.sampling.penalty_repeat = 1.1f; 
    params.sampling.temp = 0.5f;
    // params.sampling.top_k = 40;

    cactus::cactus_context ctx;
    std::cout << "Loading primary TTS model: " << params.model.path << std::endl;
    if (!ctx.loadModel(params)) {
        std::cerr << "Failed to load primary TTS model." << std::endl;
        return 1;
    }

    if (!params.vocoder.model.path.empty()) {
        std::cout << "Loading vocoder model: " << params.vocoder.model.path << std::endl;
        if (!ctx.loadVocoderModel(params.vocoder)) {
            std::cerr << "Failed to load vocoder model." << std::endl;
            return 1; 
        }
    } else {
        std::cout << "No vocoder model path specified. TTS might fail if vocoder GGUF is required." << std::endl;
    }

    // Initialize the sampling context (needed for the primary TTS model's code generation)
    std::cout << "Initializing sampling context..." << std::endl;
    if (!ctx.initSampling()) {
        std::cerr << "Failed to initialize sampling context." << std::endl;
        return 1;
    }

    // Synthesize speech
    std::cout << "Synthesizing speech for prompt: '" << params.prompt << "' to " << output_wav_filename << "..." << std::endl;
    if (ctx.synthesizeSpeech(params.prompt, output_wav_filename)) {
        std::cout << "Speech synthesized successfully to " << output_wav_filename << std::endl;
        std::cout << "You can try playing it with a command like: aplay " << output_wav_filename << " (on Linux) or open " << output_wav_filename << " (on macOS)" << std::endl;
    } else {
        std::cerr << "Failed to synthesize speech." << std::endl;
        return 1;
    }

    std::cout << "Program finished successfully." << std::endl;
    return 0;
}