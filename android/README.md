Open Android Studio
Sync Gradle by clicking the elephant symbol on the top right panel

./gradlew clean
./gradlew assemble 

test all changes with the test-app 
we use a tiny and dumb 13m model for test, do not expect meaningful response, but you can replace url with a different model!

 ./gradlew publishReleasePublicationToMavenLocal 

 add as 

// Consumer's settings.gradle.kts
repositories {
    google()
    mavenCentral()
    maven {
        name = "GitHubPackagesCactusCompute"
        url = uri("https://maven.pkg.github.com/cactus-compute/cactus")
    }
}

// Consumer's build.gradle.kts
    dependencies {
        implementation("io.github.cactus-compute:cactus-android:0.0.1")
    }