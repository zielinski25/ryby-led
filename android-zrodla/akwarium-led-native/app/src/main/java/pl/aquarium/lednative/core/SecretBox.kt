package pl.aquarium.lednative.core

import android.content.Context
import android.security.keystore.KeyGenParameterSpec
import android.security.keystore.KeyProperties
import android.util.Base64
import javax.crypto.Cipher
import javax.crypto.KeyGenerator
import javax.crypto.SecretKey
import javax.crypto.spec.GCMParameterSpec

/** AES/GCM w Android Keystore — do refresh tokenu Firebase. */
object SecretBox {
    private const val PREF = "aquarium_native_secrets"
    private const val ALIAS = "aquarium_led_native_v1"

    fun write(context: Context, name: String, value: String) {
        if (value.isBlank()) return
        val cipher = Cipher.getInstance("AES/GCM/NoPadding")
        cipher.init(Cipher.ENCRYPT_MODE, key())
        val packed = cipher.iv + cipher.doFinal(value.toByteArray(Charsets.UTF_8))
        context.getSharedPreferences(PREF, Context.MODE_PRIVATE).edit()
            .putString(name, Base64.encodeToString(packed, Base64.NO_WRAP)).apply()
    }

    fun read(context: Context, name: String): String {
        val encoded = context.getSharedPreferences(PREF, Context.MODE_PRIVATE)
            .getString(name, null) ?: return ""
        return try {
            val packed = Base64.decode(encoded, Base64.NO_WRAP)
            val cipher = Cipher.getInstance("AES/GCM/NoPadding")
            cipher.init(
                Cipher.DECRYPT_MODE, key(),
                GCMParameterSpec(128, packed.copyOfRange(0, 12))
            )
            String(cipher.doFinal(packed.copyOfRange(12, packed.size)), Charsets.UTF_8)
        } catch (_: Exception) {
            ""
        }
    }

    fun clear(context: Context, name: String) {
        context.getSharedPreferences(PREF, Context.MODE_PRIVATE).edit().remove(name).apply()
    }

    private fun key(): SecretKey {
        val ks = java.security.KeyStore.getInstance("AndroidKeyStore").apply { load(null) }
        (ks.getKey(ALIAS, null) as? SecretKey)?.let { return it }
        val generator = KeyGenerator.getInstance(KeyProperties.KEY_ALGORITHM_AES, "AndroidKeyStore")
        generator.init(
            KeyGenParameterSpec.Builder(
                ALIAS, KeyProperties.PURPOSE_ENCRYPT or KeyProperties.PURPOSE_DECRYPT
            )
                .setBlockModes(KeyProperties.BLOCK_MODE_GCM)
                .setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_NONE)
                .setRandomizedEncryptionRequired(true)
                .build()
        )
        return generator.generateKey()
    }
}
