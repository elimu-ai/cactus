import { TextInput, Button, View } from "react-native"

export const MessageField = ({ 
    message, 
    setMessage, 
    handleSendMessage, 
    isGenerating 
}: { message: string, setMessage: (text: string) => void, handleSendMessage: () => void, isGenerating: boolean }) => {
    return (
        <View style={{ padding: 10, flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between' }}>
          <TextInput 
            placeholder="Message" 
            value={message} 
            style={{ 
                backgroundColor: '#FFFFFF', 
                flex: 1, 
                height: '100%', 
                borderRadius: 10, 
                padding: 10 
            }} 
            onChangeText={setMessage}
        />
          <Button 
            title="Send" 
            onPress={handleSendMessage} 
            disabled={isGenerating}
        />
        </View>
    )
}