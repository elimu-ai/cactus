import { View, Text } from "react-native"

export interface Message {
    role: 'user' | 'assistant';
    content: string;
}
  
export const MessageBubble = ({ message }: { message: Message }) => {
    return (
        <Text
            style={{
            flex: 1,
            backgroundColor: "#EEEEEE",
            textAlign: message.role === 'user' ? 'right' : 'left',
            borderRadius: 10,
            width: 'auto',
            maxWidth: '80%',
            marginLeft: message.role === 'user' ? 'auto' : '2%',
            marginRight: message.role === 'user' ? '2%' : 'auto',
            marginBottom: '2%',
            padding: '2%',
            fontSize: 16,
            color: '#000000',
            fontStyle: message.role === 'user' ? 'normal' : 'italic'
            }}
        >
            {message.content}
        </Text>
    )
}