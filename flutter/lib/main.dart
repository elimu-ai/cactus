import 'package:flutter/material.dart';
import 'cactus_flutter.dart';
import 'package:path_provider/path_provider.dart';
import 'dart:io';
import 'package:path/path.dart' as path;

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Cactus Demo',
      theme: ThemeData(
        primarySwatch: Colors.green,
      ),
      home: const MyHomePage(title: 'Cactus C++ Integration Demo'),
    );
  }
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({Key? key, required this.title}) : super(key: key);

  final String title;

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  String _status = 'Not initialized';
  bool _isLoading = false;

  Future<void> _initializeModel() async {
    setState(() {
      _isLoading = true;
      _status = 'Initializing...';
    });

    try {
      // In a real app, you would have your model file in assets and copy it to a readable location
      final appDir = await getApplicationDocumentsDirectory();
      final modelPath = path.join(appDir.path, 'model.bin');
      
      // This is a placeholder for actual model initialization
      // You'd need to copy your model file to modelPath first
      final result = await CactusFlutter.initializeModel(modelPath);
      
      setState(() {
        _status = result == 0 
            ? 'Model initialized successfully!' 
            : 'Initialization failed with code: $result';
      });
    } catch (e) {
      setState(() {
        _status = 'Error: ${e.toString()}';
      });
    } finally {
      setState(() {
        _isLoading = false;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(widget.title),
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: <Widget>[
            Text(
              'Status:',
              style: Theme.of(context).textTheme.headlineSmall,
            ),
            Padding(
              padding: const EdgeInsets.all(16.0),
              child: Text(
                _status,
                style: Theme.of(context).textTheme.bodyLarge,
                textAlign: TextAlign.center,
              ),
            ),
            if (_isLoading)
              const CircularProgressIndicator(),
          ],
        ),
      ),
      floatingActionButton: FloatingActionButton(
        onPressed: _isLoading ? null : _initializeModel,
        tooltip: 'Initialize Model',
        child: const Icon(Icons.play_arrow),
      ),
    );
  }
} 