# Cactus for Flutter

A lightweight, high-performance framework for running AI models on mobile devices with Flutter. Cactus enables on-device inference, ensuring privacy and offline capabilities for your Flutter applications.

## Features

*   **Easy Model Initialization**: Load GGUF models from local paths or download from a URL.
*   **Text Completion**: Generate text based on prompts, with support for token streaming.
*   **Chat Interaction**: Supports multi-turn chat conversations using a familiar message structure.
*   **Customizable Inference**: Fine-tune generation with parameters like temperature, top_k, top_p, and various penalty options.
*   **Embedding Generation**: Create vector embeddings from text for semantic search, clustering, and more.
*   **Tokenization Utilities**: Directly tokenize and detokenize text.
*   **Hardware Acceleration**: Supports GPU acceleration (via `nGpuLayers`) and other performance options like `mmap`, `mlock`, and `flashAttn`.
*   **Cross-Platform**: Designed for both Android and iOS.

## Installation

1.  Add `cactus` to your `pubspec.yaml` dependencies:

    ```yaml
    dependencies:
      cactus: ^0.0.3
    ```

2.  Run `flutter pub get`.

    The necessary native libraries (`libcactus.so` for Android and `cactus.framework` or `libcactus.dylib` for iOS) are bundled with the plugin and will be automatically included in your application during the build process.

## Basic Usage

### Initialize a Model

```dart
import 'package:cactus/cactus.dart';
import 'dart:io' show Directory; // For path_provider if using model download
import 'package:path_provider/path_provider.dart'; // For model download

CactusContext? cactusContext;

Future<void> initializeModel() async {
  try {
    // Option 1: From a local path
    // final initParams = CactusInitParams(
    //   modelPath: '/path/to/your/model.gguf',
    //   nCtx: 512,
    //   nThreads: 4,
    // );

    // Option 2: From a URL (will be downloaded to app's document directory)
    final initParams = CactusInitParams(
      modelUrl: 'YOUR_MODEL_URL_HERE', // e.g., https://huggingface.co/.../phi-2.Q4_K_M.gguf
      // modelFilename: 'phi-2.Q4_K_M.gguf', // Optional: specify a local filename
      nCtx: 512,
      nThreads: 4,
      onInitProgress: (progress, message, isError) {
        print('Init Progress: $message (${progress != null ? (progress * 100).toStringAsFixed(1) + '%' : 'N/A'})');
        if (isError) {
          print('Initialization Error: $message');
        }
      },
    );

    cactusContext = await CactusContext.init(initParams);
    print('CactusContext initialized successfully!');
  } catch (e) {
    print('Failed to initialize CactusContext: $e');
  }
}
```

### Text Completion (Chat)

```dart
Future<void> performChatCompletion() async {
  if (cactusContext == null) {
    print('Context not initialized');
    return;
  }

  final messages = [
    ChatMessage(role: 'system', content: 'You are a helpful AI assistant.'),
    ChatMessage(role: 'user', content: 'Explain quantum computing in simple terms.'),
  ];

  final completionParams = CactusCompletionParams(
    messages: messages,
    temperature: 0.7,
    nPredict: 256,
    onNewToken: (token) {
      // Process each token as it's generated (for streaming)
      print('New token: $token');
      return true; // Return false to stop generation early
    },
  );

  try {
    print('Starting completion...');
    final result = await cactusContext!.completion(completionParams);
    print('Completion finished.');
    print('Generated Text: ${result.text}');
    print('Tokens Predicted: ${result.tokensPredicted}');
    print('Stopped by EOS: ${result.stoppedEos}');
  } catch (e) {
    print('Error during completion: $e');
  }
}
```

### Clean Up

Always release the context when you're done with it to free up native resources.

```dart
void disposeContext() {
  cactusContext?.free();
  cactusContext = null;
  print('CactusContext freed.');
}
```

## API Overview

### `CactusInitParams`
Parameters for initializing the `CactusContext`.
Key fields:
*   `modelPath`: Path to a model file on the device.
*   `modelUrl`: URL to download the model from.
*   `modelFilename`: Optional filename for the downloaded model.
*   `chatTemplate`: Custom chat template string (default is ChatML).
*   `nCtx`: Context size.
*   `nBatch`: Batch size for prompt processing.
*   `nGpuLayers`: Number of layers to offload to GPU (0 for CPU only).
*   `nThreads`: Number of threads for CPU inference.
*   `useMmap`, `useMlock`: Memory mapping options.
*   `embedding`: Set to true if the model is primarily for embeddings.
*   `poolingType`, `embdNormalize`: Embedding options.
*   `onInitProgress`: Callback for initialization and download progress.

### `CactusContext`
The main class for interacting with a loaded model.
*   `static Future<CactusContext> init(CactusInitParams params)`: Initializes and loads a model.
*   `Future<CactusCompletionResult> completion(CactusCompletionParams params)`: Performs text/chat completion.
*   `List<int> tokenize(String text)`: Converts text to tokens.
*   `String detokenize(List<int> tokens)`: Converts tokens back to text.
*   `List<double> embedding(String text)`: Generates embeddings for the given text.
*   `void stopCompletion()`: Asynchronously requests the current completion to stop.
*   `void free()`: Releases all native resources associated with the context.

### `CactusCompletionParams`
Parameters for a completion request.
Key fields:
*   `messages`: A list of `ChatMessage` objects for chat-style completion.
*   `nPredict`: Max number of tokens to predict.
*   `temperature`, `topK`, `topP`, `minP`, `typicalP`: Sampling parameters.
*   `penaltyLastN`, `penaltyRepeat`, `penaltyFreq`, `penaltyPresent`: Repetition penalty parameters.
*   `mirostat`, `mirostatTau`, `mirostatEta`: Mirostat sampling parameters.
*   `stopSequences`: A list of strings that, when encountered, will stop generation.
*   `grammar`: GBNF grammar for constrained generation.
*   `onNewToken`: Callback function that receives each new token as it's generated. Return `false` from the callback to stop generation.

### `ChatMessage`
*   `role`: String (`system`, `user`, or `assistant`).
*   `content`: String message content.

### `CactusCompletionResult`
The result of a completion operation.
*   `text`: The generated text.
*   `tokensPredicted`: Number of tokens predicted.
*   `tokensEvaluated`: Number of prompt tokens evaluated.
*   `stoppedEos`, `stoppedWord`, `stoppedLimit`: Booleans indicating why generation stopped.
*   `stoppingWord`: The specific word from `stopSequences` that caused generation to stop, if any.

### `downloadModel()` (Top-level function)
Exposed for direct use, but `CactusContext.init` handles downloads internally if `modelUrl` is provided.
`Future<void> downloadModel(String url, String filePath, {void Function(double? progress, String statusMessage)? onProgress})`

## Model Management

*   **URL-based**: If `modelUrl` is provided in `CactusInitParams`, the model will be downloaded to the application's documents directory (obtained via `path_provider`). The `modelFilename` parameter can be used to specify the name of the downloaded file.
*   **Path-based**: If `modelPath` is provided, Cactus will attempt to load the model directly from that path. Ensure your app has the necessary permissions to access this path and that the model file exists.
*   Models should be in GGUF format.

## Error Handling

Wrap calls to `CactusContext.init()` and `cactusContext.completion()` in `try-catch` blocks to handle potential errors during model loading or inference.

```dart
try {
  // Your Cactus operation
} catch (e) {
  print('An error occurred: $e');
  // Handle the error appropriately
}
```

## Best Practices

1.  **Resource Management**: Always call `cactusContext.free()` when you are finished with a model instance to release native memory and resources.
2.  **Asynchronous Operations**: Initialization and completion are `async`. Use `await` and manage them appropriately within your Flutter app's lifecycle.
3.  **Progress Indication**: Use the `onInitProgress` callback to provide feedback to users during model downloads and initialization, which can be time-consuming.
4.  **Token Streaming**: For responsive UIs, use the `onNewToken` callback in `CactusCompletionParams` to display text as it's generated rather than waiting for the full completion.
5.  **Thread Management**: The `nThreads` parameter in `CactusInitParams` controls CPU thread usage. Adjust based on target devices and desired performance vs. battery trade-off. `nGpuLayers` can offload work to the GPU if available and supported by the underlying llama.cpp build.

## Example App

(Consider adding a link to a simple example Flutter application in your repository if you create one. This greatly helps users get started.)

## License

This project is licensed under the [MIT License](https://github.com/cactus-compute/cactus/blob/main/LICENSE). 
