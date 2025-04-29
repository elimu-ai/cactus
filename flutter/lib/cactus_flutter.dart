import 'dart:async';
import 'package:flutter/foundation.dart';
import 'cactus_ffi.dart';

/// The Cactus Flutter plugin for running AI models locally on mobile devices
class CactusFlutter {
  /// Initialize the Cactus engine with a model file
  ///
  /// Returns a status code (0 for success, non-zero for failure)
  static Future<int> initializeModel(String modelPath) async {
    try {
      return compute(_initInIsolate, modelPath);
    } catch (e) {
      // Fallback to main thread if compute fails
      return Cactus.initialize(modelPath);
    }
  }
  
  /// Helper function to run initialization in an isolate
  static int _initInIsolate(String modelPath) {
    return Cactus.initialize(modelPath);
  }
  
  /// Add more methods that provide functionality from your C++ library
  /// For example:
  /// static Future<String> generateText(String prompt) async {...}
  /// static Future<void> unloadModel() async {...}
} 