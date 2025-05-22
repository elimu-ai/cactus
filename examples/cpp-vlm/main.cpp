// Standard C++ libraries for input/output, strings, vectors, file operations, and system calls.
#include <iostream>
#include <string>
#include <vector>
#include <fstream> // Required for std::ifstream (file input)
#include <cstdlib> // Required for system() (executing shell commands)
#include <cassert> // Required for assert() (debugging checks)
#include <cstring> // Required for C-style string functions

// Main header file for the Cactus library.
#include "../../cactus/cactus.h"

// --- Helper function to check if a file exists ---
// Takes a file path as a string and returns true if the file exists and is readable, false otherwise.
bool fileExists(const std::string& filepath) {
    std::ifstream f(filepath.c_str()); // Attempt to open the file for reading.
    return f.good(); // f.good() returns true if the stream is in a good state.
}

// --- Function to download a file using curl ---
// Takes a URL, a desired local file path, and a descriptive name for the file (for messages).
// Returns true if the file already exists or is downloaded successfully, false on failure.
bool downloadFile(const std::string& url, const std::string& filepath, const std::string& filename) {
    // Check if the file already exists to avoid re-downloading.
    if (fileExists(filepath)) {
        std::cout << filename << " already exists at " << filepath << std::endl;
        return true;
    }

    // Inform the user about the download.
    std::cout << "Downloading " << filename << " from " << url << " to " << filepath << "..." << std::endl;
    // Construct the curl command.
    std::string command = "curl -L -o \"" + filepath + "\" \"" + url + "\"";
    
    // Execute the command.
    int return_code = system(command.c_str());

    // Check for download success.
    if (return_code == 0 && fileExists(filepath)) {
        std::cout << filename << " downloaded successfully." << std::endl;
        return true;
    } else {
        // Handle download failure.
        std::cerr << "Failed to download " << filename << "." << std::endl;
        std::cerr << "Please ensure curl is installed and the URL is correct." << std::endl;
        std::cerr << "You can try downloading it manually using the command:" << std::endl;
        std::cerr << command << std::endl;
        
        // If a partial file was created, attempt to remove it.
        if (fileExists(filepath)) {
            std::remove(filepath.c_str());
        }
        return false;
    }
}

// Main function of the VLM example program.
int main(int argc, char **argv) {
    // --- Model Definition and Download ---
    // URL for the VLM's main language model weights (GGUF format).
    const std::string model_url = "https://huggingface.co/ggml-org/SmolVLM-500M-Instruct-GGUF/resolve/main/SmolVLM-500M-Instruct-Q8_0.gguf";
    // Local filename for the main model.
    const std::string model_filename = "SmolVLM-500M-Instruct-Q8_0.gguf";
    
    // URL for the VLM's multimodal projector weights (GGUF format).
    // The projector maps image features into the language model's embedding space.
    const std::string mmproj_url = "https://huggingface.co/ggml-org/SmolVLM-500M-Instruct-GGUF/resolve/main/mmproj-SmolVLM-500M-Instruct-Q8_0.gguf";
    // Local filename for the multimodal projector.
    const std::string mmproj_filename = "mmproj-SmolVLM-500M-Instruct-Q8_0.gguf";

    // Download the main VLM model file.
    if (!downloadFile(model_url, model_filename, "VLM model")) {
        return 1; // Exit with an error code if download fails.
    }

    // Download the multimodal projector file.
    if (!downloadFile(mmproj_url, mmproj_filename, "Multimodal projector")) {
        return 1; // Exit with an error code if download fails.
    }
    
    // --- Parameter Setup for VLM ---
    // `common_params` struct holds settings for model loading, processing, and generation.
    common_params params;
    // Path to the main language model GGUF file.
    params.model.path = model_filename;
    // Path to the multimodal projector GGUF file.
    params.mmproj.path = mmproj_filename;
    // Path to the image file(s) to be processed. Can add multiple images to the vector.
    // Ensure this image path is correct relative to where you run the executable.
    params.image.push_back("../image.jpg"); 
    // Prompt for the VLM. Typically includes a placeholder for the image (e.g., <__image__> or <image>)
    // and the text query related to the image.
    // The format "USER: ... ASSISTANT:" is a common convention for chat/instruction models.
    params.prompt = "USER: <__image__>\nDescribe this image in detail.\nASSISTANT:";
    
    // --- General Generation Control Parameters ---
    // `n_predict`: Maximum number of tokens to generate in the response.
    params.n_predict = 100;
    // `n_ctx`: Context window size. (e.g., 2048 or 4096). Should be appropriate for the model.
    params.n_ctx = 2048;
    // `n_batch`: Logical batch size for prompt processing.
    params.n_batch = 512;
    // `cpuparams.n_threads`: Number of CPU threads for computation.
    params.cpuparams.n_threads = 4;
    // `use_mmap`: Use memory mapping for faster model loading.
    params.use_mmap = true;
    // `warmup`: Perform a short warmup run. Can be useful for GPU initialization.
    params.warmup = false;

    // --- Chat Templating (Optional, Model Specific) ---
    // Some VLMs might use specific chat templates. If so, enable them like in the LLM example.
    // e.g., params.enable_chat_template = true; params.use_jinja = true;
    // Or, some models might have a named template string:
    // params.chat_template = "smolvlm"; // This is model-specific, check model card.

    // --- Cactus Context Initialization for VLM ---
    cactus::cactus_context ctx;
    // Load the VLM model and projector with the specified parameters.
    assert(ctx.loadModel(params) && "Model loading failed");
    // Initialize the sampling context.
    assert(ctx.initSampling() && "Sampling initialization failed");

    // --- Prompt Processing and Completion ---
    // Load the prompt (which includes image placeholders and text) into the model's context.
    // Cactus handles embedding the image(s) specified in params.image.
    ctx.loadPrompt();
    // Signal readiness to begin generating completion tokens.
    ctx.beginCompletion();

    // String to accumulate the full response.
    std::string response;
    // Get a pointer to the model's vocabulary.
    const llama_vocab * vocab = llama_model_get_vocab(ctx.model);
    // Loop to generate tokens.
    while (ctx.has_next_token) {
        auto tok = ctx.nextToken();
        if (tok.tok < 0) break; // Error or special condition.
        if (tok.tok == llama_vocab_eos(vocab)) break; // End-Of-Sequence token.

        // Buffer for token text.
        char buffer[64];
        // Convert token ID to string.
        int length = llama_token_to_piece(vocab, tok.tok, buffer, sizeof(buffer), false, false);
        if (length > 0) {
            response += std::string(buffer, length);
        }
    }

    // --- Post-Generation ---
    assert(!response.empty() && "Response should not be empty");
    // Print the generated response.
    std::cout << "Response: " << response << std::endl;
    std::cout << "Basic completion test passed" << std::endl;

    // Program finished successfully.
    return 0;
}