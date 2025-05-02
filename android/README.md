Open Android Studio
Sync Gradle by clicking the elephant symbol on the top right panel

./gradlew clean
./gradlew assemble 

test all changes with the toy app!

 ./gradlew publishReleasePublicationToMavenLocal 

 add as 

 dependencies {
        implementation("com.example.cactus:cactus-android:0.0.1")
    }