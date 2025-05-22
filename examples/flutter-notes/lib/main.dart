import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'dart:convert';
import 'models/note.dart';
import 'services/inference.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Flutter Demo',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple),
      ),
      home: const MyHomePage(),
    );
  }
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key});

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  List<Note> _notes = [];
  Note? _selectedNote;
  final TextEditingController _noteContentController = TextEditingController();
  final TextEditingController _noteTitleController = TextEditingController();
  final GlobalKey<ScaffoldState> _scaffoldKey = GlobalKey<ScaffoldState>(); // Add this key

  @override
  void initState() {
    super.initState();
    _loadNotes();
  }

  @override
  void dispose() {
    _noteContentController.dispose();
    _noteTitleController.dispose();
    super.dispose();
  }

  Future<void> _loadNotes() async {
    final prefs = await SharedPreferences.getInstance();
    final notesJson = prefs.getStringList('notes');
    if (notesJson != null) {
      setState(() {
        _notes = notesJson.map((json) => Note.fromJson(jsonDecode(json))).toList();
        if (_notes.isNotEmpty) {
          _selectedNote = _notes.first; // Select the first note initially
          _updateControllers();
        }
      });
    }
  }

  Future<void> _saveNotes() async {
    final prefs = await SharedPreferences.getInstance();
    final notesJson = _notes.map((note) => jsonEncode(note.toJson())).toList();
    await prefs.setStringList('notes', notesJson);
  }

  void _addNote() {
    setState(() {
      final newNote = Note(title: 'Untitled Note', content: '');
      _notes.add(newNote);
      _selectNote(newNote);
      _saveNotes();
    });
  }

  void _deleteNote(Note note) {
    setState(() {
      _notes.remove(note);
      if (_notes.isEmpty) {
        _selectedNote = null;
        _noteTitleController.clear();
        _noteContentController.clear();
      } else {
        //if the deleted note was the selected note.
        if (_selectedNote == note){
          _selectedNote = _notes.first;
          _updateControllers();
        }

      }
      _saveNotes();
    });
  }

    void _selectNote(Note note) {
      setState(() {
        _selectedNote = note;
        _updateControllers();
      });
      if (_scaffoldKey.currentState != null) { 
        _scaffoldKey.currentState!.closeDrawer(); 
      }
    }

    void _generateTitle(Note note) {
      // return;
      note.titleGenerationInProgress = true;
      LLMModel.create()
        .then((model) => {
        model.summarize(
          note.content, 
          (String partialTitle) => {
            setState(() {
              note.updateTitle(partialTitle);
              note.isTitleAiGenerated = true;
              _noteTitleController.text = partialTitle;
            })
          },
          (String generatedTitle) => {
            setState(() {
              note.updateTitle(generatedTitle);
              note.isTitleAiGenerated = true;
              _noteTitleController.text = generatedTitle;
              _saveNotes();
              note.titleGenerationInProgress = false;
            })
          }
        )
      });
    }

    void _updateNote(String text) {
      if (_selectedNote != null) {
        _selectedNote!.updateContent(text);
        _selectedNote!.lastEdited = DateTime.now();
        _saveNotes(); 
        if (_selectedNote!.isTitleAiGenerated == false && text.length > 50 && _selectedNote!.titleGenerationInProgress == false) {
          _generateTitle(_selectedNote!);
        }
      }
    }

    void _updateControllers() {
      _noteTitleController.text = _selectedNote?.title ?? '';
      _noteContentController.text = _selectedNote?.content ?? '';
    }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      key: _scaffoldKey,
      appBar: AppBar(
        backgroundColor: Colors.black,
        title: Column(
          children: [
            if (_selectedNote?.isTitleAiGenerated == false) ...[
                Text(_selectedNote?.title ?? 'Notes', style: const TextStyle(color: Colors.white)),
                const Text("Continue typing to have the title AI-generated", style: TextStyle(color: Colors.grey, fontSize: 12, fontStyle: FontStyle.italic)),
              ] else ...[
                Text(_selectedNote?.title ?? 'Notes', style: const TextStyle(color: Colors.white)),
              ],
          ],
        ),
        iconTheme: IconThemeData(color: Colors.white),
        actions: [ 
            IconButton(
            icon: const Icon(Icons.refresh), 
            onPressed: () {
              _generateTitle(_selectedNote!);
            },
          ),
        ],
      ),
      drawer: Drawer(
        child: ListView(
          padding: EdgeInsets.only(top: 0),
          children: <Widget>[
            Container(
              height: 118,
              margin: const EdgeInsets.all(0),
              decoration: const BoxDecoration(color: Colors.black),
              child: Align(
                alignment: Alignment.bottomCenter,
                child: Padding(
                  padding: const EdgeInsets.only(bottom: 16.0),
                  child: Text(
                    'Notes',
                    style: TextStyle(color: Colors.white, fontSize: 24),
                  ),
                ),
              ),
            ),
            ..._notes.map((note) {
              return ListTile(
                title: Text(note.title.isEmpty ? "Untitled" : note.title),
                onTap: () => _selectNote(note),
                trailing: IconButton(
                icon: const Icon(Icons.delete),
                onPressed: () => _deleteNote(note),
              ),
              selected: _selectedNote == note, 
              selectedColor: Colors.black,
              );
            }),
            ListTile(
              title: const Text('Add New Note'),
              onTap: _addNote,
            ),
          ],
        ),
      ),
      body: Center(
        child: Padding(
          padding: const EdgeInsets.all(16.0), 
          child: Column(
            mainAxisAlignment: MainAxisAlignment.start,
            crossAxisAlignment: CrossAxisAlignment.stretch, 
            children: <Widget>[
              Expanded( 
                child: TextField(
                  controller: _noteContentController,
                  style: TextStyle(
                    fontSize: 16.0, 
                    color: Colors.black, 
                  ),
                  maxLines: null, 
                  expands: true, 
                  decoration: const InputDecoration(
                    border: InputBorder.none, 
                    hintText: 'Note',
                    hintStyle: TextStyle(color: Colors.grey),
                  ),
                  onChanged:(text) => _updateNote(text),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
