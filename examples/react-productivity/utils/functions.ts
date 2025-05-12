import * as FileSystem from 'expo-file-system';
import { Platform } from 'react-native';


export const getModelDirectory = () => 
    Platform.OS === 'ios' 
      ? `${FileSystem.documentDirectory}local-models/`
      : `${FileSystem.cacheDirectory}local-models/`;
  
  export const getFullModelPath = (fileName: string) => 
    `${getModelDirectory()}${fileName}`;
  
  export const getModelNameFromUrl = (modelUrl: string) => 
    modelUrl.split('/').pop() || 'model.gguf'
  
  export const downloadModelIfNotExists = async (modelUrl: string): Promise<boolean> => {
    const modelName = getModelNameFromUrl(modelUrl)
    const fullModelPath = getFullModelPath(modelName);
    console.log('fullModelPath', fullModelPath)
    if (!(await FileSystem.getInfoAsync(getModelDirectory())).exists) {
      console.log('Model directory does not exist');
      await FileSystem.makeDirectoryAsync(getModelDirectory(), {
        intermediates: true
      });
      console.log('Created model directory');
    } else {
      console.log('Model directory exists');
    }
    if ((await FileSystem.getInfoAsync(fullModelPath)).exists) {
      console.log('Model file exists');
      return true;
    }
    console.log('downloading model...')
    const downloadResult = await FileSystem.downloadAsync(modelUrl, fullModelPath)
    console.log('downloadResult', downloadResult)
    return true;
  }
  
  export interface DiaryEntry {
    id: string;
    text: string;
    date: Date;
    title: string;
    visibleChars?: number;
  }


interface ParameterDetail {
  name: string;
  type: string;
  description: string;
  required: boolean;
}


export function createToolSchema(func: Function, description: string, parameterDetails: ParameterDetail[]) {
  if (typeof func !== 'function') {
    throw new Error("The 'func' argument must be a function.");
  }
  if (typeof description !== 'string' || description.trim() === '') {
    throw new Error("The 'description' argument must be a non-empty string.");
  }
  if (!Array.isArray(parameterDetails)) {
    throw new Error("The 'parameterDetails' argument must be an array.");
  }

  const properties: Record<string, any> = {};
  const requiredParams: string[] = [];

  for (const param of parameterDetails) {
    if (!param.name || !param.type || !param.description) {
      throw new Error("Each parameter detail must include 'name', 'type', and 'description'.");
    }
    properties[param.name] = {
      type: param.type,
      description: param.description,
    };
    if (param.required) {
      requiredParams.push(param.name);
    }
  }

  const toolSchema = {
    type: "function",
    function: {
      name: func.name,
      description: description,
      parameters: {
        type: "object",
        properties: properties,
      },
    },
  };

  // Only include the 'required' array if there are actually required parameters.
  if (requiredParams.length > 0) {
    toolSchema.function.parameters.required = requiredParams;
  }

  return toolSchema;
}