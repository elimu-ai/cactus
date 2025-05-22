// import 'dart:io';

// import 'package:path_provider/path_provider.dart'; 

/// Callback type for monitoring the progress of context initialization and model downloads.
///
/// [progress] is a value between 0.0 and 1.0 indicating download progress, or null if progress is indeterminate.
/// [statusMessage] provides a textual description of the current status.
/// [isError] is true if the status message represents an error.
typedef OnInitProgress = void Function(double? progress, String statusMessage, bool isError);

/// Parameters for initializing a [CactusContext].
class CactusInitParams {
  /// Path to the model file on the local device.
  ///
  /// Either [modelPath] or [modelUrl] must be provided.
  final String? modelPath;

  /// URL from which to download the model file.
  ///
  /// If provided, the model will be downloaded to the application's documents directory.
  /// Either [modelPath] or [modelUrl] must be provided.
  final String? modelUrl;

  /// Optional filename to use when saving a model downloaded via [modelUrl].
  ///
  /// If null or empty, the filename is inferred from the last segment of the [modelUrl].
  /// A default name like "downloaded_model.gguf" is used if inference fails.
  final String? modelFilename;

  /// Optional path to a multimodal projector file (e.g., for LLaVA models).
  /// This is used in conjunction with the main model file for vision capabilities.
  final String? mmprojPath;

  /// Optional custom chat template string (e.g., a Jinja2-like format).
  ///
  /// If null or empty, the native layer will attempt to use a default template
  /// appropriate for the loaded model (often ChatML-based).
  /// Providing a custom template overrides the model's default.
  final String? chatTemplate;

  /// The context size (often referred to as `n_ctx`) for the model.
  ///
  /// This defines the maximum number of tokens the model can consider at once from the input sequence.
  /// Larger values allow for longer context but increase memory usage and processing time.
  /// Defaults to 512.
  final int contextSize;

  /// The batch size (often `n_batch`) for prompt processing.
  ///
  /// Defines how many tokens are processed in parallel during the initial prompt ingestion.
  /// Larger values can sometimes improve throughput but increase memory usage.
  /// Defaults to 512.
  final int batchSize;

  /// The ubatch size (often `n_ubatch`) for prompt processing.
  ///
  /// Typically related to batch processing, often similar to [batchSize].
  /// Defaults to 512.
  final int ubatchSize;

  /// Number of GPU layers to offload for models that support it.
  ///
  /// Offloading layers to the GPU can significantly speed up inference.
  /// A value of 0 means CPU-only inference. Positive values indicate the number of layers.
  /// The optimal number depends on the model and available VRAM.
  /// Defaults to 0.
  final int gpuLayers;

  /// Number of threads to use for computation on the CPU.
  ///
  /// More threads can speed up CPU-bound tasks but may lead to diminishing returns or contention.
  /// Defaults to 4.
  final int threads;

  /// Whether to use `mmap` (memory-mapping) for model loading if supported by the OS.
  ///
  /// `mmap` can speed up model loading by mapping the model file directly into memory
  /// instead of reading it sequentially.
  /// Defaults to true.
  final bool useMmap;

  /// Whether to use `mlock` to prevent the model from being swapped out of RAM.
  ///
  /// `mlock` can improve performance consistency by ensuring the model stays in physical memory,
  /// but it also means that memory is persistently allocated and cannot be used by other processes.
  /// Use with caution, especially on memory-constrained devices.
  /// Defaults to false.
  final bool useMlock;

  /// Whether the model is primarily intended for generating embeddings.
  ///
  /// Set this to true if you plan to mainly use the [CactusContext.embedding] method.
  /// This might influence how the model is configured by the native layer.
  /// Defaults to false.
  final bool generateEmbeddings;

  /// Pooling type to use when [generateEmbeddings] is true.
  ///
  /// This determines how token-level embeddings are aggregated into a single sequence embedding.
  /// Common values:
  /// - 0: Unspecified (native layer will use its default, often LLAMA_POOLING_TYPE_NONE or LLAMA_POOLING_TYPE_CLS depending on model).
  /// - 1: `LLAMA_POOLING_TYPE_NONE` (no pooling, typically embedding of the last token or a specific token like BOS/EOS).
  /// - 2: `LLAMA_POOLING_TYPE_MEAN` (average of token embeddings).
  /// - 3: `LLAMA_POOLING_TYPE_CLS` (embedding of the CLS token, if present and trained for).
  /// - 4: `LLAMA_POOLING_TYPE_LAST` (embedding of the last token).
  /// - 5: `LLAMA_POOLING_TYPE_RANK` (rank-based pooling, less common).
  /// Defaults to 0.
  final int poolingType;

  /// Whether to normalize embeddings when [generateEmbeddings] is true.
  ///
  /// Normalizing embeddings to unit length can be beneficial for some downstream tasks.
  /// - 0: False (do not normalize).
  /// - 1: True (normalize to unit length).
  /// Defaults to 1 (true).
  final int normalizeEmbeddings;

  /// Whether to enable Flash Attention if available and supported by the model and hardware.
  ///
  /// Flash Attention is an optimized attention mechanism that can significantly speed up
  /// inference and reduce memory usage for transformer models.
  /// Defaults to false.
  final bool useFlashAttention;

  /// The K cache type for the model's key-value cache (e.g., "f16", "q8_0").
  ///
  /// This affects the data type used for storing the "keys" in the attention mechanism's
  /// KV cache. Lower precision (e.g., "q8_0") can save memory but may slightly affect quality.
  /// If null, the native side determines the default.
  final String? cacheTypeK;

  /// The V cache type for the model's key-value cache (e.g., "f16", "q8_0").
  ///
  /// Similar to [cacheTypeK], but for the "values" in the KV cache.
  /// If null, the native side determines the default.
  final String? cacheTypeV;

  /// Callback for receiving progress updates during [CactusContext.init],
  /// which includes model download (if applicable) and native context setup.
  final OnInitProgress? onInitProgress;

  /// Creates parameters for [CactusContext] initialization.
  ///
  /// At least one of [modelPath] or [modelUrl] must be provided.
  /// Throws an [ArgumentError] if neither is provided, or if both are provided.
  CactusInitParams({
    this.modelPath,
    this.modelUrl,
    this.modelFilename,
    this.mmprojPath,
    this.chatTemplate,
    this.contextSize = 512,
    this.batchSize = 512,
    this.ubatchSize = 512,
    this.gpuLayers = 0,
    this.threads = 4,
    this.useMmap = true,
    this.useMlock = false,
    this.generateEmbeddings = false,
    this.poolingType = 0,
    this.normalizeEmbeddings = 1,
    this.useFlashAttention = false,
    this.cacheTypeK,
    this.cacheTypeV,
    this.onInitProgress,
  }) {
    if (modelPath == null && modelUrl == null) {
      throw ArgumentError('Either modelPath or modelUrl must be provided.');
    }
    if (modelPath != null && modelUrl != null) {
      throw ArgumentError('Cannot provide both modelPath and modelUrl. Choose one.');
    }
  }
} 