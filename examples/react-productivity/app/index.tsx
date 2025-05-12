import { ThemedView } from '@/components/ThemedView';
import { downloadModelIfNotExists, getFullModelPath, getModelNameFromUrl } from '@/utils/functions';
import { styles } from '@/utils/styles';
import { Tools } from '@/utils/tools';
import { LlamaContext, initLlama } from 'cactus-react-native';
import * as FileSystem from 'expo-file-system';
import { Calendar, Check, LucideIcon, Mail, X } from 'lucide-react-native';
import React, { useEffect, useState } from 'react';
import { FlatList, Platform, Text, TextInput, TouchableOpacity, View } from 'react-native';
import Animated, { FadeInLeft, LinearTransition } from 'react-native-reanimated';
import uuid from 'react-native-uuid';

interface RecommendedAction {
  id: string;
  title: string;
  description: string;
  icon: LucideIcon;
  accepted: boolean;
}

export default function HomeScreen() {
  const [context, setContext] = useState<LlamaContext | null>(null);
  const [modelDownloaded, setModelDownloaded] = useState(false);
  const [modelLoaded, setModelLoaded] = useState(false);
  const [latestCharAnalysed, setLatestCharAnalysed] = useState(0);
  const [recommendedActions, setRecommendedActions] = useState<RecommendedAction[]>([]);
  const [inferenceInProgress, setInferenceInProgress] = useState(false);
  
  const modelUrl = 'https://huggingface.co/Mungert/gemma-3-4b-it-qat-q4_0-GGUF/resolve/main/gemma-3-4b-it-qat-q4_0-q3_k_s.gguf'

  const downloadModel = async () => {return await downloadModelIfNotExists(modelUrl)}

  const loadModel = async () => {
    const modelPath = getFullModelPath(getModelNameFromUrl(modelUrl));
    if ((await FileSystem.getInfoAsync(modelPath)).exists) {
      const context = await initLlama({
      model: modelPath,
      use_mlock: true,
      n_ctx: 2048,
        n_gpu_layers: Platform.OS === 'ios' ? 99 : 0
      });
      setContext(context);
    }
  }

  useEffect(() => {
    downloadModel().then((result) => {
      setModelDownloaded(result);
    })
    console.log(JSON.stringify(Tools.getSchemas(), null, 2));
  }, []);

  useEffect(() => {
    if (modelDownloaded) {
      loadModel().then(() => {
        setModelLoaded(true);
      })
    }
  }, [modelDownloaded])

  Tools.add(
    async function setReminder(date: string, message: string) {
      setRecommendedActions([...recommendedActions, {id: uuid.v4(), title: 'Set a reminder', description: message, icon: Calendar, accepted: false}]);
    },
    "Suggests to set a reminder for a specific date. Use this if the user's input indicates that they would want to be reminded of something at a specific date and time. If in doubt, call!",
    {
      date: {
        type: "string",
        description: "The date and time to set the reminder for"
      },
      message: {
        type: "string",
        description: "The message to set the reminder for"
      }
    }
  );

  Tools.add(
    async function writeDraftEmail(subject: string) {
      setRecommendedActions([...recommendedActions, {id: uuid.v4(), title: 'Write a draft email', description: subject, icon: Mail, accepted: false}]);
    },
    "Suggests to write a draft email. Use this if you think you'll save the user time by writing a draft email. If in doubt, call!",
    {
      subject: {
        type: "string",
        description: "The subject of the email",
        required: true
      }
    },
  );

  const invokeLLM = async (entry: string) => {
    if(!context) return;
    setInferenceInProgress(true);
    console.log('Invoking completion...');
    const fullPrompt = `You are a personal assistant for a productivity app. The user message represents the note they are typing. You have access to the following functions. Use them if required - 
    ${JSON.stringify(Tools.getSchemas())}
    Only use the an available tool if needed. If a tool is chosen, respond ONLY with a JSON object matching the following schema:
    \`\`\`json
    {
      "tool_name": "<name of the tool>",
      "tool_input": {
        "<parameter_name>": "<parameter_value>",
        ...
      }
    }
    \`\`\`
    Remember, if you are calling a tool, you must respond with the JSON object and the JSON object ONLY!!!
    If no tool is needed, respond normally.
    `

    const result = await context.completion({
      messages: [
        {role: 'system', content: fullPrompt},
        {role: 'user', content: entry}
      ],
      temperature: 0.7,
      n_predict: 100,
      // tools: tools,
      tool_choice: 'auto',
      stop: ['<end_of_turn>']
    });

    // console.log('Result:', result);

    // if (result.content.startsWith('```json')) {
    const match = result.content.match(/```json\s*([\s\S]*?)\s*```/);
    if (match) {
      try {
        const jsonContent = JSON.parse(match[1]);
        const { tool_name, tool_input } = jsonContent;
        console.log('Calling tool:', tool_name, tool_input);
        const result = await Tools.execute(tool_name, tool_input);
        console.log('Tool called result:', result);
      } catch (error) {
        console.error('Error parsing JSON:', error);
      }
    } else {
      // console.log('No tool called');
    }
    setInferenceInProgress(false);
  }

  const handleInputChange = async (text: string) => {
    if (text.trim() === '') return;
    if (!modelLoaded) return;
    if (!context) return;
    if (inferenceInProgress) return;

    const lastChar = text.charAt(text.length - 1);
    if (lastChar === '\n' || lastChar === '.') {
      // console.log('Trigger LLM!');
      const textToAnalyse = text.substring(latestCharAnalysed);
      const result = await invokeLLM(textToAnalyse);
      // console.log('Result:', result);
      setLatestCharAnalysed(text.length);
      // Handle the enter key press here
    }
  };

  return (
    <ThemedView style={styles.container}>
      <TextInput
        style={[styles.input, { borderWidth: 0 }]}
        placeholder="Write your thoughts..."
        onChangeText={handleInputChange}
        multiline
        numberOfLines={15}
      />
      <Text style={{ fontWeight: 'bold', marginBottom: 10 }}>Recommended actions</Text>
      {recommendedActions.length === 0 && <Text style={{ marginBottom: 10 }}>Start typing to get recommendations...</Text>}

      <FlatList
        data={recommendedActions}
        renderItem={({ item }) => (
          <Animated.View entering={FadeInLeft.duration(500)} layout={LinearTransition.springify().damping(10).stiffness(100).mass(0.4)} style={{ width: '100%', marginBottom: 10, padding: 10, flexDirection: 'row', justifyContent: 'flex-start', backgroundColor: '#EEEEEE', borderRadius: 10 }}>
            <View style={{ flexDirection: 'row', alignItems: 'center' }}>
              <item.icon size={24} color="black" style={{ marginRight: 10 }}/>
            </View>
            <View style={{ flexDirection: 'column', justifyContent: 'space-between' }}>
              <Text style={{ fontWeight: 'bold', marginBottom: 0 }}>{item.title}</Text>
              <Text>{item.description.length > 30 ? `${item.description.substring(0, 30)}...` : item.description}</Text>
            </View>
            <View style={{ flex: 1 }} />
            {item.accepted ? (
              <View style={{ flexDirection: 'row', justifyContent: 'flex-end', alignItems: 'center', marginRight: 8 }}>
                <Check size={16} color="black" />
              </View>
            ) : ( 
              <View style={{ flexDirection: 'row', justifyContent: 'flex-end', alignItems: 'center' }}>
                <TouchableOpacity style={{ marginRight: 10, padding: 8, backgroundColor: '#FF6B6B', borderRadius: 5 }} onPress={() => setRecommendedActions(recommendedActions.filter(action => action.id !== item.id))}>
                  <X size={16} color="white" />
                </TouchableOpacity>
                <TouchableOpacity style={{ padding: 8, backgroundColor: '#4CAF50', borderRadius: 5 }} onPress={() => setRecommendedActions(recommendedActions.map((action, idx) => action.id === item.id ? {...action, id: uuid.v4(), accepted: true} : action))}>
                  <Check size={16} color="white"/>
                </TouchableOpacity>
              </View>
            )}
          </Animated.View>
        )}
        keyExtractor={(item) => item.id}
        numColumns={1}
      />
      {/* <Button title="Pop one in" onPress={() => setRecommendedActions([...recommendedActions, {id: uuid.v4(), title: `Write a draft email ${uuid.v4().substring(0, 5)}`, description: 'Hello', icon: Mail, accepted: false}])} /> */}
    </ThemedView>
  );
}
