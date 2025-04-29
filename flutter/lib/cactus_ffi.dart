import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';
import 'package:path/path.dart' as path;
import 'package:path_provider/path_provider.dart';

/// Loads the native library based on the platform
DynamicLibrary _loadLibrary() {
  if (Platform.isAndroid) {
    return DynamicLibrary.open('libcactus.so');
  } else if (Platform.isIOS) {
    return DynamicLibrary.process();
  } else {
    throw UnsupportedError('Unsupported platform: ${Platform.operatingSystem}');
  }
}

/// The dynamic library in which the functions are stored
final DynamicLibrary _lib = _loadLibrary();

/// Example of defining a function from your native library
/// Replace with actual functions from your C++ code
final initCactus = _lib.lookupFunction<
    Int32 Function(Pointer<Utf8>),
    int Function(Pointer<Utf8>)>('initCactus');

/// A Dart class to provide a nice interface to the native code
class Cactus {
  /// Initialize the Cactus engine
  static int initialize(String modelPath) {
    final pathPointer = modelPath.toNativeUtf8();
    try {
      return initCactus(pathPointer);
    } finally {
      calloc.free(pathPointer);
    }
  }
  
  /// Add more methods that interface with your native library
  /// For example:
  /// static Future<String> runInference(String input) {...}
  /// static void cleanup() {...}
} 