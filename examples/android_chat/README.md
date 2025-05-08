# Cactus Android Chat Example

This application demonstrates how to use the Cactus Android library to build a simple chat interface powered by an on-device Large Language Model (LLM).

## Overview

The Android Chat Example showcases:
*   Initializing the Cactus `LlamaContext`.
*   Performing text completion for chat-like interactions.
*   Streaming responses token by token.
*   Basic UI for sending messages and displaying responses.
*   Managing the model lifecycle.

## Prerequisites

*   **Android Studio:** Latest stable version recommended.
*   **Android SDK:** API level 26 or higher.
*   **Android NDK:** Required for the Cactus library's native components. Android Studio should prompt you to install the necessary version if it's missing.
*   **GGUF Model Information:** The application is configured to automatically download a default GGUF-formatted LLM. You can typically find information about this model (e.g., its source URL) within the application's source code (e.g., in a configuration file or ViewModel). If you wish to use a different model, you can modify the download URL in the code or manually place your own model file (see 'Model Management' below).
*   **GitHub Personal Access Token (PAT):** To download the Cactus Android library dependency, as it's hosted on GitHub Packages. Refer to the main [Cactus Android README](../../android/README.md#important-credentials-required-for-github-packages) for instructions on generating and setting up your PAT.

## Setup Instructions

1.  **Clone the Cactus Repository (if you haven't already):**
    ```bash
    git clone https://github.com/cactus-compute/cactus.git
    cd cactus/examples/android_chat
    ```

2.  **Open in Android Studio:**
    *   Open Android Studio.
    *   Select "Open" and navigate to the `cactus/examples/android_chat` directory.
    *   Allow Android Studio to sync Gradle and download dependencies. You might be prompted to install missing SDK or NDK components.

3.  **Configure GitHub Package Credentials:**
    *   Ensure you have set up your GitHub username and PAT as described in the prerequisites. Typically, this involves creating or editing the `local.properties` file in the **root directory of your Android project** (i.e., `examples/android_chat/local.properties`).
    *   Add the following lines to `local.properties`, replacing `YOUR_GITHUB_USERNAME` and `YOUR_PAT`:
        ```properties
        gpr.user=YOUR_GITHUB_USERNAME
        gpr.key=YOUR_PAT
        ```
    *   Make sure `local.properties` is in your `.gitignore` file.

4.  **Model Management:**
    *   **Default Model Download:** This example application is designed to automatically download a default GGUF model upon its first run if the model is not found in its expected local storage location. The specific model and download URL are typically defined within the application's source code (e.g., in a `ViewModel`, `Repository`, or configuration constants file).
    *   The downloaded model will usually be saved to the app's internal storage (e.g., in the `files` directory).
    *   **Initial Model Setup Time:** Be aware that the first time you run the app, downloading and setting up the model might take some time depending on its size and your network speed. Subsequent runs should be faster as the model will be loaded from local storage.
    *   **Using a Custom Model (Optional):**
        If you wish to use a different GGUF model, you have a couple of options:
        1.  **Modify Download Source (Code Change):**
            *   Locate the part of the application code where the model download URL is defined (e.g., in a `ChatViewModel.kt`, a repository class, or a constants file).
            *   Change the URL to point to your desired GGUF model.
            *   Clean and rebuild the app. It should then attempt to download your specified model. Ensure it's a GGUF format compatible with Cactus.
        2.  **Manual Placement (Alternative for advanced users or testing):**
            *   If you want to use a model file already on your device or place it manually, bypassing the download logic:
                a.  You would typically need to modify the app's code to look for the model at a specific path you define, or copy it from assets to internal storage and then point the `LlamaInitParams` to that path.
                b.  **Example: Copying from assets (if you add a model to `app/src/main/assets/`):**
                    In your model initialization logic (e.g., in a ViewModel or Repository where `LlamaContext` is created):
                    ```kotlin
                    // Example: Ensure you have context available, e.g., from applicationContext
                    // val modelName = "your_custom_model.gguf" // Your model's name in the assets folder
                    // val filesDir = context.filesDir // Get the app's files directory
                    // val outputFile = File(filesDir, modelName)
                    //
                    // if (!outputFile.exists()) { // Check if model already copied
                    //    try {
                    //        context.assets.open(modelName).use { inputStream ->
                    //            outputFile.outputStream().use { outputStream ->
                    //                inputStream.copyTo(outputStream)
                    //            }
                    //        }
                    //        Log.i("ModelSetup", "Copied model from assets: ${'$'}{outputFile.absolutePath}")
                    //    } catch (e: IOException) {
                    //        Log.e("ModelSetup", "Error copying model from assets", e)
                    //        // Handle error: model might not be found or other IO issues
                    //    }
                    // }
                    // val modelPath = outputFile.absolutePath
                    ```
                c.  Then, update the `modelPath` in `LlamaInitParams` to this `modelPath`:
                    ```kotlin
                    // LlamaInitParams(
                    //    modelPath = modelPath, // Path to the model (copied from assets or direct path)
                    //    // ... other parameters
                    // )
                    ```
                d. This often requires understanding and potentially modifying the example's existing model loading/downloading logic to correctly prioritize your manually placed model. It's recommended to check the example's code (e.g., `ChatViewModel.kt` or any model handling classes) for details on how it manages model paths.

Refer to the application's specific code (e.g., `ChatViewModel.kt`, `ModelDownloader.kt`, or similar) to understand its exact model handling logic before making changes.

## Building and Running

1.  **Select a Device/Emulator:**
    *   Connect an Android device (with USB debugging enabled) or choose an emulator (API 26+).
2.  **Build and Run:**
    *   Click the "Run" button (green play icon) in Android Studio or select "Run > Run 'app'" from the menu.
    *   The app should build, install, and launch on the selected device/emulator.

3.  **Interact with the Chat:**
    *   Once the model is loaded (which might take some time on the first run or with large models), you can type messages into the input field and receive responses from the LLM.

## Features Demonstrated

*   **Model Initialization:** How to set up `LlamaInitParams` and create a `LlamaContext`.
*   **Text Completion:** Using `llamaContext.complete()` for generating chat responses.
*   **Streaming:** Receiving and displaying tokens as they are generated using `PartialCompletionCallback`.
*   **Coroutine Usage:** Performing model operations on background threads using Kotlin Coroutines (`Dispatchers.IO`).
*   **Basic Android UI:** Simple `EditText` for input and `TextView` or `RecyclerView` for displaying the chat history.

## Troubleshooting

*   **Model Load Failure:**
    *   Ensure the `modelPath` in `LlamaInitParams` is correct and the app has permissions to access it.
    *   Check Android Studio's Logcat for error messages from "LlamaInit" or "CactusFramework". The logs might indicate issues with the model file itself or parameters.
    *   Ensure your model is a GGUF format compatible with the underlying `llama.cpp` version used by Cactus.
*   **Credential Issues for Dependency:**
    *   If Gradle sync fails with errors related to `maven.pkg.github.com`, double-check your `gpr.user` and `gpr.key` in `local.properties` and ensure your PAT has the `read:packages` scope.
*   **App Crashes or ANR (Application Not Responding):**
    *   Verify that all Cactus operations (initialization, completion, etc.) are performed off the main UI thread (e.g., within a `viewModelScope.launch(Dispatchers.IO)` block).
    *   Large models can be memory-intensive. Test on devices with sufficient RAM.
*   **Slow Performance:**
    *   Use quantized models (e.g., Q4_K_M) for better performance on mobile devices.
    *   Adjust `nThreads` in `LlamaInitParams` (e.g., to half the number of available cores).

This example provides a starting point. For more advanced features and best practices, refer to the main [Cactus Android README](../../android/README.md). 