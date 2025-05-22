// Standard C++ libraries for input/output, strings, vectors, file operations, and system calls.
#include <iostream>
#include <string>
#include <vector>
#include <fstream> // Required for std::ifstream (file input)
#include <cstdlib> // Required for system() (executing shell commands)
#include <cassert> // Required for assert() (debugging checks)
#include <cstring> // Required for C-style string functions (though not directly used in this version)

// Main header file for the Cactus library. This should be in your include path.
#include "../../cactus/cactus.h"

// --- Helper function to check if a file exists ---
// Takes a file path as a string and returns true if the file exists and is readable, false otherwise.
bool fileExists(const std::string& filepath) {
    std::ifstream f(filepath.c_str()); // Attempt to open the file for reading.
    return f.good(); // f.good() returns true if the stream is in a good state (e.g., file opened successfully).
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
    // -L: Follow redirects.
    // -o: Output to file (filepath).
    std::string command = "curl -L -o \"" + filepath + "\" \"" + url + "\"";
    
    // Execute the command using system().
    int return_code = system(command.c_str());

    // Check if the download was successful.
    // return_code == 0 usually indicates success for shell commands.
    // Also, verify that the file now exists.
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

// Main function of the program.
int main(int argc, char **argv) {
    // --- Model Definition and Download ---
    // URL from which to download the GGUF model.
    // GGUF is a file format for storing language models for inference with llama.cpp and similar engines.
    const std::string model_url = "https://huggingface.co/lm-kit/qwen-3-0.6b-instruct-gguf/resolve/main/Qwen3-0.6B-Q6_K.gguf";
    // Local filename for the downloaded model.
    const std::string model_filename = "Qwen3-0.6B-Q6_K.gguf";
    
    // Download the model file if it doesn't already exist.
    // Exits if download fails.
    if (!downloadFile(model_url, model_filename, "LLM")) {
        return 1; // Return an error code.
    }
    
    // --- Parameter Setup ---
    // `common_params` is a struct (defined in common.h, part of llama.cpp) that holds various settings
    // for model loading, processing, and generation.
    common_params params;

    // Path to the GGUF model file on the local system.
    params.model.path = model_filename;

    // --- Chat Templating ---
    // Many models, especially instruction-tuned or chat models, expect input in a specific format.
    // `enable_chat_template` and `use_jinja` instruct Cactus/llama.cpp to use the
    // chat template defined within the GGUF model file (if available).
    // The Qwen model series often uses a specific template (like ChatML).
    params.enable_chat_template = true;
    params.use_jinja = true; // Use Jinja2 templating if the model specifies it.

    // --- System and User Prompts ---
    // The system prompt provides high-level instructions or persona for the AI.
    // The `/no_think` suffix is a Qwen-specific directive to encourage more direct responses.
    params.system_prompt = "Directly answer the user's question and nothing else. Do not add any commentary, notes, or explanations. Provide only the answer to the question. /no_think";
    // The user prompt is the actual query or instruction for the current turn.
    params.prompt = "What is the main cause of Earth's seasons? /no_think";

    // --- Generation Control Parameters ---
    // `n_predict`: Maximum number of tokens to generate in the response.
    // Set to -1 for no limit (or until other stopping conditions are met).
    params.n_predict = 64; 
    // `n_ctx`: Context window size in tokens. The model can "see" this many tokens back.
    // This value can impact memory usage and performance. 0 means use the model's default.
    params.n_ctx = 512;
    // `n_batch`: Logical batch size for prompt processing.
    params.n_batch = 512;
    // `cpuparams.n_threads`: Number of CPU threads to use for computation.
    params.cpuparams.n_threads = 4; // Adjust based on your CPU cores.
    // `use_mmap`: Use memory mapping to load the model. Can speed up loading and reduce RAM for shared models.
    params.use_mmap = true;
    // `warmup`: Perform a short warmup run before the actual generation. Can be useful for Metal (GPU).
    params.warmup = false;

    // --- Sampling Parameters ---
    // These parameters control how the next token is chosen from the model's probability distribution.
    // Different models/tasks may benefit from different sampling strategies.
    // NOTE: Always check the model card for the best sampling parameters!!!
    // `temp`: Temperature. Higher values (e.g., 0.8-1.0) make output more random/creative.
    // Lower values (e.g., 0.2-0.5) make it more deterministic/focused. 0.0 means greedy decoding.
    params.sampling.temp = 0.7f; 
    // `top_k`: Top-K sampling. Considers only the K most probable tokens. 0 to disable.
    params.sampling.top_k = 20;
    // `top_p`: Top-P (nucleus) sampling. Considers the smallest set of tokens whose cumulative probability exceeds P. 1.0 to disable.
    params.sampling.top_p = 0.8f; 
    // `min_p`: Min-P sampling. Filters out tokens with probability below P * max_probability. 0.0 to disable.
    params.sampling.min_p = 0.0f; 
    // `penalty_present`: Presence penalty. Penalizes tokens already present in the generated text. 0.0 to disable.
    params.sampling.penalty_present = 1.5f; 
    // `penalty_last_n`: Number of recent tokens to consider for repetition penalties.
    params.sampling.penalty_last_n = 512; 

    // --- Advanced Parameters ---
    // `reasoning_format`: Some models use specific tags for "thinking" or reasoning steps (e.g., DeepSeek).
    // Setting to NONE disables special parsing for these.
    params.reasoning_format = COMMON_REASONING_FORMAT_NONE;
    // `ctx_shift`: Context shifting for very long generations. Disabled here.
    params.ctx_shift = false; 

    // --- Antiprompts (Currently Disabled by User) ---
    // Antiprompts are strings that, if generated by the model, will cause generation to stop.
    // Useful for preventing verbose "thinking" or unwanted boilerplate.
    // Example: params.antiprompt.push_back("\nOkay, I think");
    // Example: params.antiprompt.push_back("USER:"); // Stop if model tries to hallucinate user input

    // --- Cactus Context Initialization ---
    // `cactus::cactus_context` is the main object for interacting with the model.
    cactus::cactus_context ctx;
    // Load the model with the specified parameters. Asserts ensure this succeeds.
    assert(ctx.loadModel(params) && "Model loading failed");
    // Initialize the sampling context.
    assert(ctx.initSampling() && "Sampling initialization failed");

    // --- Prompt Processing and Completion ---
    // Load the combined system and user prompt into the model's context.
    ctx.loadPrompt();
    // Signal that we are ready to begin generating completion tokens.
    ctx.beginCompletion();

    // Prepare for streaming output.
    std::cout << "Response: " << std::flush; 
    // String to accumulate the full response, e.g., for assertion or later use.
    std::string full_response_for_assert; 

    // Get a pointer to the model's vocabulary for token-to-text conversion.
    const llama_vocab * vocab = llama_model_get_vocab(ctx.model);
    // Loop to generate tokens until the model indicates completion or an error occurs.
    while (ctx.has_next_token) {
        // Get the next token from the model.
        auto tok = ctx.nextToken();
        // Negative token ID might indicate an error or special condition.
        if (tok.tok < 0) break;
        // Check for the End-Of-Sequence (EOS) token.
        if (tok.tok == llama_vocab_eos(vocab)) break;

        // Buffer to hold the text representation of the token.
        char buffer[64];
        // Convert the token ID to its string representation.
        int length = llama_token_to_piece(vocab, tok.tok, buffer, sizeof(buffer), false, false);
        if (length > 0) {
            // Print the token's text to the console immediately (streaming).
            std::cout << std::string(buffer, length) << std::flush;
            // Append to the full response string.
            full_response_for_assert += std::string(buffer, length);
        }
    }
    // Add a newline after the full response has been streamed.
    std::cout << std::endl; 

    // --- Post-Generation ---
    // Assert that some response was generated.
    assert(!full_response_for_assert.empty() && "Response should not be empty");
    // Indicate that the basic test passed.
    std::cout << "Basic completion test passed" << std::endl;

    // Program finished successfully.
    return 0;
}