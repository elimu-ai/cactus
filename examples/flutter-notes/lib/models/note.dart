class Note {
  String title;
  String content;
  bool titleGenerationInProgress;
  bool isTitleAiGenerated;
  DateTime lastEdited;

  Note({required this.title, required this.content}) : 
    lastEdited = DateTime.now(), isTitleAiGenerated = false, titleGenerationInProgress = false;

  void updateContent(String newContent) {
    content = newContent;
    lastEdited = DateTime.now();
  }

  void updateTitle(String newTitle) {
    title = newTitle;
    lastEdited = DateTime.now();
  }

  Map<String, dynamic> toJson() {
    return {
      'title': title,
      'content': content,
      'lastEdited': lastEdited.toIso8601String(),
    };
  }

  factory Note.fromJson(Map<String, dynamic> json) {
    return Note(
      title: json['title'] ?? '',
      content: json['content'] ?? '',
    )..lastEdited = DateTime.parse(json['lastEdited']);
  }
}