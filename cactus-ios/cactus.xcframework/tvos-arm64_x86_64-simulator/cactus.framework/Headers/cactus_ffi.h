#ifndef CACTUS_FFI_H
#define CACTUS_FFI_H

#include <stdint.h>
#include <stdbool.h>

// Define export macro
#if defined _WIN32 || defined __CYGWIN__
  #ifdef CACTUS_FFI_BUILDING_DLL // Define this when building the DLL
    #ifdef __GNUC__
      #define CACTUS_FFI_EXPORT __attribute__ ((dllexport))
    #else
      #define CACTUS_FFI_EXPORT __declspec(dllexport)
    #endif
  #else
    #ifdef __GNUC__
      #define CACTUS_FFI_EXPORT __attribute__ ((dllimport))
    #else
      #define CACTUS_FFI_EXPORT __declspec(dllimport)
    #endif
  #endif
  #define CACTUS_FFI_LOCAL
#else // For non-Windows (Linux, macOS, Android)
  #if __GNUC__ >= 4
    #define CACTUS_FFI_EXPORT __attribute__ ((visibility ("default")))
    #define CACTUS_FFI_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define CACTUS_FFI_EXPORT
    #define CACTUS_FFI_LOCAL
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cactus_context_opaque* cactus_context_handle_t;


typedef struct cactus_init_params_c {
    const char* model_path;
    const char* mmproj_path;
    const char* chat_template; 

    int32_t n_ctx;
    int32_t n_batch;
    int32_t n_ubatch;
    int32_t n_gpu_layers;
    int32_t n_threads;
    bool use_mmap;
    bool use_mlock;
    bool embedding; 
    int32_t pooling_type; 
    int32_t embd_normalize;
    bool flash_attn;
    const char* cache_type_k; 
    const char* cache_type_v; 
    void (*progress_callback)(float progress); 

} cactus_init_params_c_t;

typedef struct cactus_completion_params_c {
    const char* prompt;
    const char* image_path;
    int32_t n_predict; 
    int32_t n_threads; 
    int32_t seed;
    double temperature;
    int32_t top_k;
    double top_p;
    double min_p;
    double typical_p;
    int32_t penalty_last_n;
    double penalty_repeat;
    double penalty_freq;
    double penalty_present;
    int32_t mirostat;
    double mirostat_tau;
    double mirostat_eta;
    bool ignore_eos;
    int32_t n_probs; 
    const char** stop_sequences; 
    int stop_sequence_count;
    const char* grammar; 
    bool (*token_callback)(const char* token_json);

} cactus_completion_params_c_t;


typedef struct cactus_token_array_c {
    int32_t* tokens;
    int32_t count;
} cactus_token_array_c_t;

typedef struct cactus_float_array_c {
    float* values;
    int32_t count;
} cactus_float_array_c_t;

typedef struct cactus_completion_result_c {
    char* text; 
    int32_t tokens_predicted;
    int32_t tokens_evaluated;
    bool truncated;
    bool stopped_eos;
    bool stopped_word;
    bool stopped_limit;
    char* stopping_word; 
} cactus_completion_result_c_t;


/**
 * @brief Parameters for loading a vocoder model (mirrors internal common_params_model).
 */
typedef struct cactus_vocoder_model_params_c {
    const char* path;    // Local path to the vocoder model file
    // Add other fields like url, hf_repo, hf_file if needed for FFI-based downloading
} cactus_vocoder_model_params_c_t;


/**
 * @brief Parameters for initializing the vocoder component within a cactus_context.
 */
typedef struct cactus_vocoder_load_params_c {
    cactus_vocoder_model_params_c_t model_params; // Vocoder model details
    const char* speaker_file;                     // Path to speaker embedding file (optional)
    bool use_guide_tokens;                        // Whether to use guide tokens
} cactus_vocoder_load_params_c_t;


/**
 * @brief Parameters for speech synthesis.
 */
typedef struct cactus_synthesize_speech_params_c {
    const char* text_input;      // The text to synthesize
    const char* output_wav_path; // Path to save the output WAV file
    const char* speaker_id;      // Optional speaker ID (can be NULL or empty)
} cactus_synthesize_speech_params_c_t;


/**
 * @brief Initializes a cactus context with the given parameters.
 *
 * @param params Parameters for initialization.
 * @return A handle to the context, or NULL on failure. Caller must free with cactus_free_context_c.
 */
CACTUS_FFI_EXPORT cactus_context_handle_t cactus_init_context_c(const cactus_init_params_c_t* params);


/**
 * @brief Frees the resources associated with a cactus context.
 *
 * @param handle The context handle returned by cactus_init_context_c.
 */
CACTUS_FFI_EXPORT void cactus_free_context_c(cactus_context_handle_t handle);


/**
 * @brief Performs text completion based on the provided prompt and parameters.
 *        This is potentially a long-running operation.
 *        Tokens are streamed via the callback in params.
 *
 * @param handle The context handle.
 * @param params Completion parameters, including prompt and sampling settings.
 * @param result Output struct to store the final result details (text must be freed).
 * @return 0 on success, non-zero on failure.
 */
CACTUS_FFI_EXPORT int cactus_completion_c(
    cactus_context_handle_t handle,
    const cactus_completion_params_c_t* params,
    cactus_completion_result_c_t* result // Output parameter
);


/**
 * @brief Requests the ongoing completion operation to stop.
 *        This sets an interrupt flag; completion does not stop instantly.
 *
 * @param handle The context handle.
 */
CACTUS_FFI_EXPORT void cactus_stop_completion_c(cactus_context_handle_t handle);


/**
 * @brief Tokenizes the given text.
 *
 * @param handle The context handle.
 * @param text The text to tokenize.
 * @return A struct containing the tokens. Caller must free the `tokens` array using cactus_free_token_array_c.
 */
CACTUS_FFI_EXPORT cactus_token_array_c_t cactus_tokenize_c(cactus_context_handle_t handle, const char* text);


/**
 * @brief Detokenizes the given sequence of tokens.
 *
 * @param handle The context handle.
 * @param tokens Pointer to the token IDs.
 * @param count Number of tokens.
 * @return The detokenized string. Caller must free using cactus_free_string_c.
 */
CACTUS_FFI_EXPORT char* cactus_detokenize_c(cactus_context_handle_t handle, const int32_t* tokens, int32_t count);


/**
 * @brief Generates embeddings for the given text. Context must be initialized with embedding=true.
 *
 * @param handle The context handle.
 * @param text The text to embed.
 * @return A struct containing the embedding values. Caller must free the `values` array using cactus_free_float_array_c.
 */
CACTUS_FFI_EXPORT cactus_float_array_c_t cactus_embedding_c(cactus_context_handle_t handle, const char* text);


/**
 * @brief Loads the vocoder model required for Text-to-Speech.
 *        This should be called after cactus_init_context_c if TTS is needed.
 *        The main model (TTS model) should be loaded via cactus_init_context_c.
 *
 * @param handle The context handle returned by cactus_init_context_c.
 * @param params Parameters for loading the vocoder model.
 * @return 0 on success, non-zero on failure.
 */
CACTUS_FFI_EXPORT int cactus_load_vocoder_c(
    cactus_context_handle_t handle,
    const cactus_vocoder_load_params_c_t* params
);


/**
 * @brief Synthesizes speech from the given text and saves it to a WAV file.
 *        Both the main TTS model (via cactus_init_context_c) and the vocoder model
 *        (via cactus_load_vocoder_c) must be loaded before calling this.
 *
 * @param handle The context handle.
 * @param params Parameters for synthesis, including input text and output path.
 * @return 0 on success, non-zero on failure.
 */
CACTUS_FFI_EXPORT int cactus_synthesize_speech_c(
    cactus_context_handle_t handle,
    const cactus_synthesize_speech_params_c_t* params
);


/**
 * @brief Formats a list of chat messages using the appropriate chat template.
 *
 * @param handle The context handle.
 * @param messages_json A JSON string representing an array of chat messages (e.g., [{"role": "user", "content": "Hello"}]).
 * @param override_chat_template An optional chat template string to use. If NULL or empty,
 *                               the template from context initialization or the model's default will be used.
 * @return A newly allocated C string containing the fully formatted prompt. Caller must free using cactus_free_string_c.
 *         Returns an empty string on failure.
 */
CACTUS_FFI_EXPORT char* cactus_get_formatted_chat_c(
    cactus_context_handle_t handle,
    const char* messages_json,
    const char* override_chat_template
);


/** @brief Frees a string allocated by the C API. */
CACTUS_FFI_EXPORT void cactus_free_string_c(char* str);

/** @brief Frees a token array allocated by the C API. */
CACTUS_FFI_EXPORT void cactus_free_token_array_c(cactus_token_array_c_t arr);

/** @brief Frees a float array allocated by the C API. */
CACTUS_FFI_EXPORT void cactus_free_float_array_c(cactus_float_array_c_t arr);

/** @brief Frees the members *within* a completion result struct (like text, stopping_word). */
CACTUS_FFI_EXPORT void cactus_free_completion_result_members_c(cactus_completion_result_c_t* result);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // CACTUS_FFI_H 