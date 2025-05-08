package com.example.android_chat

import android.os.Bundle
import android.util.Log
import androidx.fragment.app.Fragment
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.lifecycle.lifecycleScope
import com.cactus.android.LlamaContext
import com.cactus.android.LlamaInitParams
import com.cactus.android.LlamaCompletionParams
import com.example.android_chat.databinding.FragmentFirstBinding
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import java.io.FileOutputStream
import java.io.InputStream
import java.net.HttpURLConnection
import java.net.URL

/**
 * A simple [Fragment] subclass as the default destination in the navigation.
 */
class FirstFragment : Fragment() {

    private var _binding: FragmentFirstBinding? = null

    // This property is only valid between onCreateView and
    // onDestroyView.
    private val binding get() = _binding!!

    private val TAG = "FirstFragment"

    // Model download constants
    private val modelUrl = "https://huggingface.co/QuantFactory/SmolLM2-135M-GGUF/resolve/main/SmolLM2-135M.Q8_0.gguf"
    private val modelFileName = "SmolLM2-135M.Q8_0.gguf"
    private lateinit var modelFile: File

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentFirstBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        // Initialize the target model file path in internal storage accessible to this app
        modelFile = File(requireContext().filesDir, modelFileName)

        binding.editTextModelPath.setText(modelFile.absolutePath) // Show the target path
        binding.buttonRunInference.setOnClickListener { runInference() }

        downloadModelIfNeeded()
    }

    private fun downloadModelIfNeeded() {
        if (modelFile.exists()) {
            Log.i(TAG, "Model already exists at: ${modelFile.absolutePath}")
            binding.textViewOutput.text = "Model found locally. Enter prompt and run inference."
            binding.buttonRunInference.isEnabled = true
            return
        }

        Log.i(TAG, "Model not found. Starting download from: $modelUrl")
        binding.textViewOutput.text = "Downloading model ($modelFileName)... Please wait."
        binding.buttonRunInference.isEnabled = false // Disable button during download

        lifecycleScope.launch(Dispatchers.IO) { // Run download in background
            var success = false
            var errorMessage = ""
            try {
                val url = URL(modelUrl)
                val connection = url.openConnection() as HttpURLConnection
                connection.connect()

                if (connection.responseCode != HttpURLConnection.HTTP_OK) {
                    throw RuntimeException("Server returned HTTP ${connection.responseCode} ${connection.responseMessage}")
                }

                val fileSize = connection.contentLength
                val inputStream: InputStream = connection.inputStream
                val outputStream = FileOutputStream(modelFile) // Save to app's internal files dir
                val buffer = ByteArray(4096)
                var bytesRead: Int
                var totalBytesRead = 0L

                while (inputStream.read(buffer).also { bytesRead = it } != -1) {
                    outputStream.write(buffer, 0, bytesRead)
                    totalBytesRead += bytesRead
                    val progress = if (fileSize > 0) (totalBytesRead * 100 / fileSize).toInt() else -1
                    withContext(Dispatchers.Main) {
                        binding.textViewOutput.text = "Downloading model... ${totalBytesRead / (1024 * 1024)} MB" +
                                if (progress >= 0) " ($progress%)" else ""
                    }
                }

                outputStream.close()
                inputStream.close()
                connection.disconnect()
                Log.i(TAG, "Model download completed successfully.")
                success = true

            } catch (e: Exception) {
                Log.e(TAG, "Model download failed", e)
                errorMessage = e.message ?: "Unknown download error"
                if (modelFile.exists()) {
                    modelFile.delete()
                }
            } finally {
                withContext(Dispatchers.Main) {
                    if (success) {
                        binding.textViewOutput.text = "Model downloaded. Ready for inference."
                        binding.buttonRunInference.isEnabled = true
                    } else {
                        binding.textViewOutput.text = "Model download failed: $errorMessage"
                        binding.buttonRunInference.isEnabled = false
                    }
                }
            }
        }
    }

    private fun runInference() {
        val modelPath = modelFile.absolutePath
        val prompt = binding.editTextPrompt.text.toString().trim()

        if (!modelFile.exists()) {
            binding.textViewOutput.text = "Error: Model file not found at: $modelPath\nTry restarting the app or view logs."
            downloadModelIfNeeded() // Offer to download again
            return
        }
        if (prompt.isEmpty()) {
            Toast.makeText(context, "Please enter a prompt", Toast.LENGTH_SHORT).show()
            return
        }

        binding.textViewOutput.text = "Loading model and running inference..."
        binding.buttonRunInference.isEnabled = false

        lifecycleScope.launch(Dispatchers.IO) {
            var resultText = "Error occurred"
            try {
                val initParams = LlamaInitParams(
                    modelPath = modelPath,
                    nCtx = 512 // Example context size
                )

                LlamaContext.create(initParams).use { context ->
                    Log.i(TAG, "LlamaContext created successfully.")

                    val completionParams = LlamaCompletionParams(
                        temperature = 0.7f,
                        nPredict = 128 // Example prediction length
                    )

                    Log.i(TAG, "Running completion with prompt: '$prompt'")
                    // Assuming doCompletionNative is the method from your library
                    // and it returns a Map<String, Any?> or similar.
                    // Adjust according to the actual signature in your LlamaContext.
                    val resultMap = LlamaContext.doCompletionNative(
                        context.contextPtr, 
                        prompt,
                        completionParams.chatFormat,
                        completionParams.grammar ?: "",
                        completionParams.grammarLazy,
                        emptyList(), // grammarTriggers
                        emptyList(), // preservedTokens
                        completionParams.temperature,
                        completionParams.nThreads,
                        completionParams.nPredict,
                        completionParams.nProbs,
                        completionParams.penaltyLastN,
                        completionParams.penaltyRepeat,
                        completionParams.penaltyFreq,
                        completionParams.penaltyPresent,
                        completionParams.mirostat,
                        completionParams.mirostatTau,
                        completionParams.mirostatEta,
                        completionParams.topK,
                        completionParams.topP,
                        completionParams.minP,
                        completionParams.xtcThreshold,
                        completionParams.xtcProbability,
                        completionParams.typicalP,
                        completionParams.seed,
                        completionParams.stop?.toTypedArray() ?: emptyArray(),
                        completionParams.ignoreEos,
                        completionParams.logitBias ?: emptyMap(),
                        completionParams.dryMultiplier,
                        completionParams.dryBase,
                        completionParams.dryAllowedLength,
                        completionParams.dryPenaltyLastN,
                        completionParams.topNSigma,
                        completionParams.drySequenceBreakers?.toTypedArray() ?: emptyArray(),
                        null // partial_completion_callback
                    )

                    if (resultMap != null) {
                        Log.i(TAG, "Completion successful (raw map): $resultMap")
                        resultText = resultMap["text"] as? String ?: "(No text found in result map)"
                    } else {
                        Log.e(TAG, "Completion failed: Native method returned null.")
                        resultText = "Completion failed: Native method returned null."
                    }
                }

            } catch (e: Throwable) {
                Log.e(TAG, "Error during Llama operation", e)
                resultText = "Error: ${e.message ?: e.toString()}"
            } finally {
                withContext(Dispatchers.Main) {
                    binding.textViewOutput.text = resultText
                    binding.buttonRunInference.isEnabled = true
                }
            }
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}