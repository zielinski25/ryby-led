plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

// Inject locally read firmware secret via environment; never persist it in source.
val rybyCmdToken = System.getenv("RYBY_CMD_TOKEN").orEmpty()

android {
    namespace = "pl.aquarium.lednative"
    compileSdk = 34

    defaultConfig {
        applicationId = "pl.aquarium.lednative"
        minSdk = 24
        targetSdk = 34
        versionCode = 1
        versionName = "1.3.0"
        buildConfigField("String", "RYBY_CMD_TOKEN", "\"${rybyCmdToken}\"")
    }

    buildTypes {
        release {
            isMinifyEnabled = false
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_1_8
        targetCompatibility = JavaVersion.VERSION_1_8
    }

    kotlinOptions {
        jvmTarget = "1.8"
    }

    buildFeatures {
        compose = true
        buildConfig = true
    }

    composeOptions {
        kotlinCompilerExtensionVersion = "1.5.4"
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.12.0")
    implementation("androidx.lifecycle:lifecycle-runtime-ktx:2.7.0")
    implementation("androidx.lifecycle:lifecycle-viewmodel-ktx:2.7.0")
    implementation("androidx.activity:activity-compose:1.8.2")
    implementation("androidx.compose.ui:ui:1.6.3")
    implementation("androidx.compose.ui:ui-text:1.6.3")
    implementation("androidx.compose.ui:ui-graphics:1.6.3")
    implementation("androidx.compose.ui:ui-tooling-preview:1.6.3")
    implementation("androidx.compose.foundation:foundation:1.6.3")
    implementation("androidx.compose.foundation:foundation-layout:1.6.3")
    implementation("androidx.compose.animation:animation:1.6.3")
    implementation("androidx.compose.material3:material3:1.2.1")
    implementation("androidx.compose.material:material-icons-extended:1.6.3")
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.7.3")
    implementation("com.squareup.okhttp3:okhttp:4.12.0")
}