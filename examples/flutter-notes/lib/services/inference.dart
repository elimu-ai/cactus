import 'package:cactus/cactus.dart';

CactusContext? cactusContext;

class LLMModel {
  String latestResult = "";
  CactusContext? cactusContext;

  LLMModel._();

  static Future<LLMModel> create() async {
    final instance = LLMModel._();
    await instance._initialize();
    return instance;
  }

  Future<void> _initialize() async {
    try {
      final initParams = CactusInitParams(
      modelUrl: 'https://huggingface.co/unsloth/SmolLM2-135M-Instruct-GGUF/resolve/main/SmolLM2-135M-Instruct-Q8_0.gguf',
      nCtx: 512,
      nThreads: 4,
      onInitProgress: (progress, message, isError) {
        print('Init Progress: $message (${progress != null ? (progress * 100).toStringAsFixed(1) + '%' : 'N/A'})');
      },
    );
    cactusContext = await CactusContext.init(initParams);
    } catch (e) {
      print("Failed to initialize model: $e");
      rethrow;
    }
  }

  Future<String> summarize(String noteText, Function(String) onPartialCallback,Function(String) onCompleteCallback) async {
    final messages = [
      ChatMessage(role: 'system', content: '''You are a specialized Note Title Generator. Your one and only task is to create a title for any note the user provides. This title MUST be EXACTLY FOUR (4) WORDS long.

CRITICAL INSTRUCTIONS - YOU MUST FOLLOW THESE:

1.  **OUTPUT LENGTH:** Your response MUST be precisely FOUR (4) words. Not three words. Not five words. EXACTLY FOUR (4) words.
2.  **OUTPUT CONTENT:** Your response must ONLY be the four-word title itself. Do NOT add any other text, greetings, or explanations. For example, DO NOT say "Here is the four-word title:" or "Okay, I will summarize this." Your entire output is just the four words of the title.
3.  **RELEVANCE:** The four-word title must be a summary or a descriptive title for the content of the user's note.

Let me be perfectly clear. This is how it works:
The user will give you a piece of text (a note).
You will process this text.
You will then output EXACTLY FOUR (4) words that act as a title for that text. AND NOTHING ELSE.
'''),
      ChatMessage(role: 'user', content: noteText),
    ];
    final completionParams = CactusCompletionParams(
      messages: messages,
      temperature: 0.7,
      nPredict: 8,
      stopSequences: ['<|im_end|>'],
      onNewToken: (token) {
        onPartialCallback(token);
        return true; // Continue generation
      },
    );
    final result = await cactusContext!.completion(completionParams);
    onCompleteCallback(result.text);
    cactusContext!.free();
    return result.text;
  }
}
