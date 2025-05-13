import { ThemedView } from '@/components/ThemedView';
import { downloadModelIfNotExists, getFullModelPath, getModelNameFromUrl } from '@/utils/functions';
import { styles } from '@/utils/styles';
import { LlamaContext, initLlama, Tools } from 'cactus-react-native';
import * as FileSystem from 'expo-file-system';
import { Calendar, Check, LucideIcon, Mail, X } from 'lucide-react-native';
import { useEffect, useState } from 'react';
import { FlatList, Platform, Text, TextInput, TouchableOpacity, View } from 'react-native';
import Animated, { FadeInLeft, LinearTransition } from 'react-native-reanimated';
import uuid from 'react-native-uuid';
import { ActivityIndicator } from 'react-native';

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
  const tools = new Tools();
  
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
  }, []);

  useEffect(() => {
    if (modelDownloaded) {
      loadModel().then(() => {
        setModelLoaded(true);
      })
    }
  }, [modelDownloaded])

  tools.add(
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

  tools.add(
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
    setInferenceInProgress(true)

    const _ = await context.completionWithTools({
      messages: [
        {role: 'user', content: entry}
      ],
      temperature: 0.7,
      n_predict: 100,
      tools: tools,
      tool_choice: 'auto',
      stop: ['<end_of_turn>']
    });

    setInferenceInProgress(false);
  }

  const handleInputChange = async (text: string) => {
    if (text.trim() === '') return;
    if (!modelLoaded) return;
    if (!context) return;
    if (inferenceInProgress) return;

    const lastChar = text.charAt(text.length - 1);
    if (lastChar === '\n' || lastChar === '.') {
      const textToAnalyse = text.substring(latestCharAnalysed);
      await invokeLLM(textToAnalyse);
      setLatestCharAnalysed(text.length);
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
      {inferenceInProgress && <ActivityIndicator size="small"/>}
    </ThemedView>
  );
}
