import { View, Text } from "react-native";

export default function Header() {
    return (
        <View style={{ alignItems: 'center', paddingTop: '2%', paddingBottom: '2%' }}>
            <Text style={{ fontSize: 24, fontFamily: 'Poppins' }}>Cactus Chat</Text>
        </View>
    )
}