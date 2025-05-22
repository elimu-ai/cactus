#include "cactus_ffi.h"
#include "cactus.h"
#include "common.h"
#include "llama.h"

#include <string>
#include <vector>
#include <stdexcept>
#include <cstring> 
#include <cstdlib> 
#include <sstream> 
#include <iostream> 


/**
 * @brief Converts a C-style array of strings to a C++ vector of strings.
 * @param arr The C-style array of strings.
 * @param count The number of strings in the array.
 * @return A std::vector<std::string> containing the strings.
 */
static std::vector<std::string> c_str_array_to_vector(const char** arr, int count) {
    std::vector<std::string> vec;
    if (arr != nullptr) {
        for (int i = 0; i < count; ++i) {
            if (arr[i] != nullptr) {
                vec.push_back(arr[i]);
            }
        }
    }
    return vec;
}


/**
 * @brief Safely duplicates a C string.
 * The caller is responsible for freeing the returned string using free().
 * @param str The std::string to duplicate.
 * @return A newly allocated C string, or nullptr if allocation fails. Returns an empty string if the input is empty.
 */
static char* safe_strdup(const std::string& str) {
    if (str.empty()) {
        char* empty_str = (char*)malloc(1);
        if (empty_str) empty_str[0] = '\0';
        return empty_str;
    }
    char* new_str = (char*)malloc(str.length() + 1);
    if (new_str) {
        std::strcpy(new_str, str.c_str());
    }
    return new_str; 
}


extern "C" {

/**
 * @brief Initializes a new cactus context with the given parameters.
 * This function loads the model and prepares it for use.
 * The caller is responsible for freeing the context using cactus_free_context_c.
 * @param params A pointer to the initialization parameters.
 * @return A handle to the created cactus context, or nullptr on failure.
 */
cactus_context_handle_t cactus_init_context_c(const cactus_init_params_c_t* params) {
    if (!params || !params->model_path) {
        return nullptr;
    }

    cactus::cactus_context* context = nullptr;
    try {
        context = new cactus::cactus_context();

        common_params cpp_params;
        cpp_params.model.path = params->model_path;
        if (params->mmproj_path) {
            cpp_params.mmproj.path = params->mmproj_path;
        }
        if (params->chat_template) {
            cpp_params.chat_template = params->chat_template;
        }
        cpp_params.n_ctx = params->n_ctx;
        cpp_params.n_batch = params->n_batch;
        cpp_params.n_ubatch = params->n_ubatch;
        cpp_params.n_gpu_layers = params->n_gpu_layers;
        cpp_params.cpuparams.n_threads = params->n_threads;
        cpp_params.use_mmap = params->use_mmap;
        cpp_params.use_mlock = params->use_mlock;
        cpp_params.embedding = params->embedding;
        cpp_params.pooling_type = static_cast<enum llama_pooling_type>(params->pooling_type);
        cpp_params.embd_normalize = params->embd_normalize;
        cpp_params.flash_attn = params->flash_attn;
        if (params->cache_type_k) {
             try {
                  cpp_params.cache_type_k = cactus::kv_cache_type_from_str(params->cache_type_k);
             } catch (const std::exception& e) {
                 std::cerr << "Warning: Invalid cache_type_k: " << params->cache_type_k << " Error: " << e.what() << std::endl;
                 delete context;
                 return nullptr;
             }
        }
        if (params->cache_type_v) {
            try {
                cpp_params.cache_type_v = cactus::kv_cache_type_from_str(params->cache_type_v);
            } catch (const std::exception& e) {
                std::cerr << "Warning: Invalid cache_type_v: " << params->cache_type_v << " Error: " << e.what() << std::endl;
                delete context;
                return nullptr;
            }
        }
        // TODO: Add translation for LoRA, RoPE params

        // Progress callback can be complex; this simple version might crash if the Dart function disappears
        if (params->progress_callback) {
            cpp_params.progress_callback = [](float progress, void* user_data) {
                auto callback = reinterpret_cast<void (*)(float)>(user_data);
                callback(progress);
                return true; 
            };
            cpp_params.progress_callback_user_data = reinterpret_cast<void*>(params->progress_callback);
        } else {
             cpp_params.progress_callback = nullptr;
             cpp_params.progress_callback_user_data = nullptr;
        }

        if (!context->loadModel(cpp_params)) {
            // loadModel logs errors internally
            delete context;
            return nullptr;
        }

        return reinterpret_cast<cactus_context_handle_t>(context);

    } catch (const std::exception& e) {
        std::cerr << "Error initializing context: " << e.what() << std::endl;
        if (context) delete context;
        return nullptr;
    } catch (...) {
        std::cerr << "Unknown error initializing context." << std::endl;
        if (context) delete context;
        return nullptr;
    }
}


/**
 * @brief Frees a cactus context that was previously created with cactus_init_context_c.
 * @param handle The handle to the cactus context to free.
 */
void cactus_free_context_c(cactus_context_handle_t handle) {
    if (handle) {
        cactus::cactus_context* context = reinterpret_cast<cactus::cactus_context*>(handle);
        delete context;
    }
}


/**
 * @brief Performs text completion using the provided context and parameters.
 * This function can stream tokens back via a callback.
 * @param handle The handle to the cactus context.
 * @param params A pointer to the completion parameters.
 * @param result A pointer to a structure where the completion result will be stored.
 *               The caller is responsible for calling cactus_free_completion_result_members_c
 *               on the result structure to free allocated memory for text and stopping_word.
 * @return 0 on success, negative value on error.
 *         -1: Invalid arguments (handle, params, or result is null).
 *         -2: Failed to initialize sampling.
 *         -3: Exception occurred during completion.
 *         -4: Unknown exception occurred.
 */
int cactus_completion_c(
    cactus_context_handle_t handle,
    const cactus_completion_params_c_t* params,
    cactus_completion_result_c_t* result 
) {
    if (!handle || !params || !params->prompt || !result) {
        return -1; 
    }
    cactus::cactus_context* context = reinterpret_cast<cactus::cactus_context*>(handle);

    // Ensure result is zero-initialized
    memset(result, 0, sizeof(cactus_completion_result_c_t));

    try {
        context->rewind();

        context->params.prompt = params->prompt;

        if (params->image_path) {
            context->params.image.clear();
            context->params.image.push_back(params->image_path);
        } else {
            context->params.image.clear();
        }

        if (params->n_threads > 0) {
             context->params.cpuparams.n_threads = params->n_threads;
        }
        context->params.n_predict = params->n_predict;
        context->params.sampling.seed = params->seed;
        context->params.sampling.temp = params->temperature;
        context->params.sampling.top_k = params->top_k;
        context->params.sampling.top_p = params->top_p;
        context->params.sampling.min_p = params->min_p;
        context->params.sampling.typ_p = params->typical_p;
        context->params.sampling.penalty_last_n = params->penalty_last_n;
        context->params.sampling.penalty_repeat = params->penalty_repeat;
        context->params.sampling.penalty_freq = params->penalty_freq;
        context->params.sampling.penalty_present = params->penalty_present;
        context->params.sampling.mirostat = params->mirostat;
        context->params.sampling.mirostat_tau = params->mirostat_tau;
        context->params.sampling.mirostat_eta = params->mirostat_eta;
        context->params.sampling.ignore_eos = params->ignore_eos;
        context->params.sampling.n_probs = params->n_probs;
        context->params.antiprompt = c_str_array_to_vector(params->stop_sequences, params->stop_sequence_count);
        if (params->grammar) {
             context->params.sampling.grammar = params->grammar;
        }

        if (!context->initSampling()) {
            return -2; 
        }
        context->beginCompletion();
        context->loadPrompt();

        // --- Streaming loop --- 
        while (context->has_next_token && !context->is_interrupted) {
            const cactus::completion_token_output token_with_probs = context->doCompletion();

            if (token_with_probs.tok == -1 && !context->has_next_token) {
                 break;
            }
            
            if (token_with_probs.tok != -1 && params->token_callback) {
                // Format token data (simple example: just the text)
                // A more complex implementation could create JSON here
                std::string token_text = common_token_to_piece(context->ctx, token_with_probs.tok);
                
                // Call the Dart callback
                bool continue_completion = params->token_callback(token_text.c_str());
                if (!continue_completion) {
                    context->is_interrupted = true; 
                    break;
                }
            }
        }

        // --- Fill final result struct --- 
        result->text = safe_strdup(context->generated_text);
        result->tokens_predicted = context->num_tokens_predicted;
        result->tokens_evaluated = context->num_prompt_tokens;
        result->truncated = context->truncated;
        result->stopped_eos = context->stopped_eos;
        result->stopped_word = context->stopped_word;
        result->stopped_limit = context->stopped_limit;
        result->stopping_word = safe_strdup(context->stopping_word);
        // TODO: Populate timings 

        context->is_predicting = false;
        return 0; // Success

    } catch (const std::exception& e) {
        // Log error
        std::cerr << "Error during completion: " << e.what() << std::endl;

        // Cleanup state
        context->is_predicting = false;
        context->is_interrupted = true; 
        return -3; 

    } catch (...) {
        // Log error
        context->is_predicting = false;
        context->is_interrupted = true;
        return -4; // Unknown exception
    }
}


/**
 * @brief Stops an ongoing completion process.
 * Sets an interruption flag in the context.
 * @param handle The handle to the cactus context.
 */
void cactus_stop_completion_c(cactus_context_handle_t handle) {
    if (handle) {
        cactus::cactus_context* context = reinterpret_cast<cactus::cactus_context*>(handle);
        context->is_interrupted = true;
    }
}


/**
 * @brief Tokenizes a given text using the context's tokenizer.
 * The caller is responsible for freeing the returned token array using cactus_free_token_array_c.
 * @param handle The handle to the cactus context.
 * @param text The C string to tokenize.
 * @return A cactus_token_array_c_t structure containing the tokens and their count.
 *         The 'tokens' field will be nullptr and 'count' 0 on failure or if input is invalid.
 */
cactus_token_array_c_t cactus_tokenize_c(cactus_context_handle_t handle, const char* text) {
    cactus_token_array_c_t result = {nullptr, 0};
    if (!handle || !text) {
        return result;
    }
    cactus::cactus_context* context = reinterpret_cast<cactus::cactus_context*>(handle);
    if (!context->ctx) { // Need the llama_context
        return result;
    }

    try {
        std::vector<llama_token> tokens_vec = ::common_tokenize(context->ctx, text, false, true);
        if (!tokens_vec.empty()) {
            result.count = tokens_vec.size();
            result.tokens = (int32_t*)malloc(result.count * sizeof(int32_t));
            if (result.tokens) {
                // Copy data
                std::copy(tokens_vec.begin(), tokens_vec.end(), result.tokens);
            } else {
                result.count = 0; // Malloc failed
            }
        }
        return result;
    } catch (const std::exception& e) {
        std::cerr << "Error during tokenization: " << e.what() << std::endl;
        return {nullptr, 0};
    } catch (...) {
        std::cerr << "Unknown error during tokenization." << std::endl;
        return {nullptr, 0};
    }
}

/**
 * @brief Detokenizes an array of tokens into a string.
 * The caller is responsible for freeing the returned C string using cactus_free_string_c.
 * @param handle The handle to the cactus context.
 * @param tokens A pointer to an array of token IDs.
 * @param count The number of tokens in the array.
 * @return A newly allocated C string representing the detokenized text.
 *         Returns an empty string on failure or if input is invalid.
 */
char* cactus_detokenize_c(cactus_context_handle_t handle, const int32_t* tokens, int32_t count) {
    if (!handle || !tokens || count <= 0) {
        return safe_strdup(""); // Return empty string
    }
    cactus::cactus_context* context = reinterpret_cast<cactus::cactus_context*>(handle);
     if (!context->ctx) {
        return safe_strdup("");
    }

    try {
        std::vector<llama_token> tokens_vec(tokens, tokens + count);
        std::string text = cactus::tokens_to_str(context->ctx, tokens_vec.cbegin(), tokens_vec.cend());
        // Print the intermediate C++ string for debugging
        std::cout << "[DEBUG cactus_detokenize_c] Intermediate std::string: [" << text << "]" << std::endl;
        return safe_strdup(text);
    } catch (const std::exception& e) {
        std::cerr << "Error during detokenization: " << e.what() << std::endl;
        return safe_strdup("");
    } catch (...) {
        std::cerr << "Unknown error during detokenization." << std::endl;
        return safe_strdup("");
    }
}


/**
 * @brief Generates an embedding for the given text.
 * Embedding mode must be enabled during context initialization.
 * The caller is responsible for freeing the returned float array using cactus_free_float_array_c.
 * @param handle The handle to the cactus context.
 * @param text The C string for which to generate the embedding.
 * @return A cactus_float_array_c_t structure containing the embedding values and their count.
 *         The 'values' field will be nullptr and 'count' 0 on failure or if embedding is not enabled.
 */
cactus_float_array_c_t cactus_embedding_c(cactus_context_handle_t handle, const char* text) {
    cactus_float_array_c_t result = {nullptr, 0};
     if (!handle || !text) {
        return result;
    }
    cactus::cactus_context* context = reinterpret_cast<cactus::cactus_context*>(handle);
    if (!context->ctx || !context->params.embedding) { 
        std::cerr << "Error: Embedding mode not enabled or context not initialized." << std::endl;
        return result;
    }

    try {
        context->rewind();
        context->params.prompt = text;
        context->params.n_predict = 0; 

        if (!context->initSampling()) { return result; }
        context->beginCompletion();
        context->loadPrompt();
        context->doCompletion(); 

        common_params dummy_embd_params;
        dummy_embd_params.embd_normalize = context->params.embd_normalize;

        std::vector<float> embedding_vec = context->getEmbedding(dummy_embd_params);

        if (!embedding_vec.empty()) {
            result.count = embedding_vec.size();
            result.values = (float*)malloc(result.count * sizeof(float));
            if (result.values) {
                std::copy(embedding_vec.begin(), embedding_vec.end(), result.values);
            } else {
                result.count = 0; 
            }
        }
        context->is_predicting = false;
        return result;

    } catch (const std::exception& e) {
        std::cerr << "Error during embedding generation: " << e.what() << std::endl;
        context->is_predicting = false;
        return {nullptr, 0};
    } catch (...) {
        std::cerr << "Unknown error during embedding generation." << std::endl;
        context->is_predicting = false;
        return {nullptr, 0};
    }
}



/**
 * @brief Frees a C string that was allocated by one of the cactus_ffi functions.
 * @param str The C string to free.
 */
void cactus_free_string_c(char* str) {
    if (str) {
        free(str);
    }
}

/**
 * @brief Frees a token array structure (the 'tokens' field) allocated by cactus_tokenize_c.
 * @param arr The token array to free.
 */
void cactus_free_token_array_c(cactus_token_array_c_t arr) {
    if (arr.tokens) {
        free(arr.tokens);
    }
    // No need to zero out arr, caller owns it
}

/**
 * @brief Frees a float array structure (the 'values' field) allocated by cactus_embedding_c.
 * @param arr The float array to free.
 */
void cactus_free_float_array_c(cactus_float_array_c_t arr) {
    if (arr.values) {
        free(arr.values);
    }
}

/**
 * @brief Frees the members of a cactus_completion_result_c_t structure that were dynamically allocated.
 * Specifically, this frees the 'text' and 'stopping_word' C strings.
 * @param result A pointer to the completion result structure whose members are to be freed.
 */
void cactus_free_completion_result_members_c(cactus_completion_result_c_t* result) {
    if (result) {
        cactus_free_string_c(result->text);
        cactus_free_string_c(result->stopping_word);
        result->text = nullptr; // Prevent double free
        result->stopping_word = nullptr;
    }
}


/**
 * @brief Loads a vocoder model into the given cactus context.
 * @param handle The handle to the cactus context.
 * @param params A pointer to the vocoder loading parameters.
 * @return 0 on success, negative value on error.
 *         -1: Invalid arguments.
 *         -2: Vocoder model loading failed.
 *         -3: Exception occurred.
 *         -4: Unknown exception occurred.
 */
int cactus_load_vocoder_c(
    cactus_context_handle_t handle,
    const cactus_vocoder_load_params_c_t* params
) {
    if (!handle || !params || !params->model_params.path) {
        std::cerr << "Error: Invalid arguments to cactus_load_vocoder_c." << std::endl;
        return -1; // Invalid arguments
    }
    cactus::cactus_context* context = reinterpret_cast<cactus::cactus_context*>(handle);

    try {
        common_params_vocoder vocoder_cpp_params;
        vocoder_cpp_params.model.path = params->model_params.path;
        
        if (params->speaker_file) {
            vocoder_cpp_params.speaker_file = params->speaker_file;
        }
        vocoder_cpp_params.use_guide_tokens = params->use_guide_tokens;

        if (!context->loadVocoderModel(vocoder_cpp_params)) {
            std::cerr << "Error: Failed to load vocoder model." << std::endl;
            return -2; // Vocoder model loading failed
        }
        return 0; // Success
    } catch (const std::exception& e) {
        std::cerr << "Exception in cactus_load_vocoder_c: " << e.what() << std::endl;
        return -3; // Exception occurred
    } catch (...) {
        std::cerr << "Unknown exception in cactus_load_vocoder_c." << std::endl;
        return -4; // Unknown exception
    }
}


/**
 * @brief Synthesizes speech from the given text input and saves it to a WAV file.
 * A vocoder model must be loaded first using cactus_load_vocoder_c.
 * @param handle The handle to the cactus context.
 * @param params A pointer to the speech synthesis parameters.
 * @return 0 on success, negative value on error.
 *         -1: Invalid arguments.
 *         -2: Speech synthesis failed.
 *         -3: Exception occurred.
 *         -4: Unknown exception occurred.
 */
int cactus_synthesize_speech_c(
    cactus_context_handle_t handle,
    const cactus_synthesize_speech_params_c_t* params
) {
    if (!handle || !params || !params->text_input || !params->output_wav_path) {
        std::cerr << "Error: Invalid arguments to cactus_synthesize_speech_c." << std::endl;
        return -1; // Invalid arguments
    }
    cactus::cactus_context* context = reinterpret_cast<cactus::cactus_context*>(handle);

    try {
        std::string text_input_str = params->text_input;
        std::string output_wav_path_str = params->output_wav_path;
        std::string speaker_id_str = params->speaker_id ? params->speaker_id : "";

        if (!context->synthesizeSpeech(text_input_str, output_wav_path_str, speaker_id_str)) {
            std::cerr << "Error: Speech synthesis failed." << std::endl;
            return -2; // Synthesis failed
        }
        return 0; // Success
    } catch (const std::exception& e) {
        std::cerr << "Exception in cactus_synthesize_speech_c: " << e.what() << std::endl;
        return -3; // Exception occurred
    } catch (...) {
        std::cerr << "Unknown exception in cactus_synthesize_speech_c." << std::endl;
        return -4; // Unknown exception
    }
}

/**
 * @brief Formats a list of chat messages using the appropriate chat template.  
 * @param handle The handle to the cactus context.
 * @param messages_json A JSON string representing an array of chat messages (e.g., [{"role": "user", "content": "Hello"}]).
 * @param override_chat_template An optional chat template string to use. If NULL or empty,
 *                               the template from context initialization or the model's default will be used.
 * @return A newly allocated C string containing the fully formatted prompt. Caller must free using cactus_free_string_c.
 *         Returns an empty string on failure.
 */
char* cactus_get_formatted_chat_c(
    cactus_context_handle_t handle,
    const char* messages_json,
    const char* override_chat_template
) {
    if (!handle || !messages_json) {
        std::cerr << "Error: Invalid arguments to cactus_get_formatted_chat_c (handle or messages_json is null)." << std::endl;
        return safe_strdup(""); // Return empty string on error to avoid crashing Dart side
    }

    cactus::cactus_context* context = reinterpret_cast<cactus::cactus_context*>(handle);
    if (!context->ctx) { // Ensure the underlying llama_context is initialized
        std::cerr << "Error: cactus_context not fully initialized (llama_context is null) in cactus_get_formatted_chat_c." << std::endl;
        return safe_strdup("");
    }

    try {
        std::string messages_std_string(messages_json);
        std::string template_to_use_std_string;

        if (override_chat_template && strlen(override_chat_template) > 0) {
            template_to_use_std_string = override_chat_template;
        } else if (!context->params.chat_template.empty()) {
            template_to_use_std_string = context->params.chat_template;
        }

        std::string formatted_prompt_std_string = context->getFormattedChat(
            messages_std_string,
            template_to_use_std_string
        );
        
        return safe_strdup(formatted_prompt_std_string);

    } catch (const std::exception& e) {
        std::cerr << "Exception in cactus_get_formatted_chat_c: " << e.what() << std::endl;
        return safe_strdup(""); 
    } catch (...) {
        std::cerr << "Unknown exception in cactus_get_formatted_chat_c." << std::endl;
        return safe_strdup("");
    }
}

} // extern "C" 