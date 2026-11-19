package pl.aquarium.lednative.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.OutlinedTextFieldDefaults
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import pl.aquarium.lednative.core.Logic
import pl.aquarium.lednative.ui.Border
import pl.aquarium.lednative.ui.ChannelColors
import pl.aquarium.lednative.ui.ChannelNames
import pl.aquarium.lednative.ui.Cyan
import pl.aquarium.lednative.ui.Surface2
import pl.aquarium.lednative.ui.Surface3
import pl.aquarium.lednative.ui.TextDim
import pl.aquarium.lednative.ui.TextHi
import pl.aquarium.lednative.ui.TextMid
import kotlin.math.roundToInt

/** Suwak jednego kanalu LED z podgladem procentowym i wartoscia 0-1023. */
@Composable
fun PwmSlider(
    index: Int,
    value: Int,
    enabled: Boolean,
    onChange: (Int) -> Unit,
    modifier: Modifier = Modifier,
    onChangeFinished: (() -> Unit)? = null
) {
    val color = ChannelColors.getOrElse(index) { Cyan }
    Column(modifier.fillMaxWidth(), verticalArrangement = Arrangement.spacedBy(2.dp)) {
        Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
            Box(
                Modifier.padding(end = 7.dp).size(8.dp)
                    .clip(RoundedCornerShape(999.dp)).background(color)
            )
            Text(
                ChannelNames.getOrElse(index) { "Kanał ${index + 1}" },
                fontSize = 8.sp, color = TextMid, modifier = Modifier.weight(1f)
            )
            Text(
                "${(value * 100.0 / Logic.PWM_MAX).roundToInt()}%",
                fontSize = 8.sp, color = color, fontWeight = FontWeight.Bold
            )
            Text(
                "  $value",
                fontSize = 7.sp, color = TextDim
            )
        }
        Slider(
            value = value.toFloat(),
            onValueChange = { onChange(it.roundToInt().coerceIn(0, Logic.PWM_MAX)) },
            onValueChangeFinished = { onChangeFinished?.invoke() },
            valueRange = 0f..Logic.PWM_MAX.toFloat(),
            enabled = enabled,
            colors = SliderDefaults.colors(
                thumbColor = color,
                activeTrackColor = color,
                inactiveTrackColor = Border,
                disabledThumbColor = TextDim,
                disabledActiveTrackColor = TextDim,
                disabledInactiveTrackColor = Border
            )
        )
    }
}

@Composable
fun NumberField(
    label: String,
    value: String,
    onChange: (String) -> Unit,
    modifier: Modifier = Modifier,
    hint: String = "",
    decimal: Boolean = false,
    enabled: Boolean = true
) {
    Column(modifier, verticalArrangement = Arrangement.spacedBy(4.dp)) {
        Text(label, fontSize = 8.sp, color = TextDim, fontWeight = FontWeight.Bold)
        OutlinedTextField(
            value = value,
            onValueChange = onChange,
            modifier = Modifier.fillMaxWidth(),
            singleLine = true,
            enabled = enabled,
            keyboardOptions = KeyboardOptions(
                keyboardType = if (decimal) KeyboardType.Decimal else KeyboardType.Number
            ),
            shape = RoundedCornerShape(12.dp),
            colors = fieldColors(),
            textStyle = MaterialTheme.typography.bodyLarge
        )
        if (hint.isNotBlank()) Text(hint, fontSize = 7.sp, color = TextDim)
    }
}

@Composable
fun TextFieldStyled(
    label: String,
    value: String,
    onChange: (String) -> Unit,
    modifier: Modifier = Modifier,
    hint: String = "",
    password: Boolean = false,
    enabled: Boolean = true
) {
    Column(modifier, verticalArrangement = Arrangement.spacedBy(4.dp)) {
        Text(label, fontSize = 8.sp, color = TextDim, fontWeight = FontWeight.Bold)
        OutlinedTextField(
            value = value,
            onValueChange = onChange,
            modifier = Modifier.fillMaxWidth(),
            singleLine = true,
            enabled = enabled,
            visualTransformation = if (password) {
                androidx.compose.ui.text.input.PasswordVisualTransformation()
            } else androidx.compose.ui.text.input.VisualTransformation.None,
            shape = RoundedCornerShape(12.dp),
            colors = fieldColors(),
            textStyle = MaterialTheme.typography.bodyLarge
        )
        if (hint.isNotBlank()) Text(hint, fontSize = 7.sp, color = TextDim)
    }
}

@Composable
private fun fieldColors() = OutlinedTextFieldDefaults.colors(
    focusedTextColor = TextHi,
    unfocusedTextColor = TextHi,
    focusedBorderColor = Cyan,
    unfocusedBorderColor = Border,
    cursorColor = Cyan,
    focusedContainerColor = Surface2,
    unfocusedContainerColor = Surface2,
    disabledContainerColor = Surface2,
    disabledTextColor = TextDim
)

@Composable
fun PrimaryButton(
    text: String,
    modifier: Modifier = Modifier,
    enabled: Boolean = true,
    onClick: () -> Unit
) {
    Button(
        onClick = onClick,
        modifier = modifier.fillMaxWidth().height(34.dp),
        enabled = enabled,
        shape = RoundedCornerShape(13.dp),
        colors = ButtonDefaults.buttonColors(
            containerColor = Cyan,
            contentColor = Color(0xFF04121C),
            disabledContainerColor = Surface3,
            disabledContentColor = TextDim
        )
    ) {
        Text(text, fontWeight = FontWeight.Bold, fontSize = 9.sp)
    }
}

@Composable
fun GhostButton(
    text: String,
    modifier: Modifier = Modifier,
    enabled: Boolean = true,
    color: Color = TextMid,
    onClick: () -> Unit
) {
    OutlinedButton(
        onClick = onClick,
        modifier = modifier.height(32.dp),
        enabled = enabled,
        shape = RoundedCornerShape(12.dp),
        border = androidx.compose.foundation.BorderStroke(1.dp, Border),
        colors = ButtonDefaults.outlinedButtonColors(contentColor = color)
    ) {
        Text(text, fontSize = 8.sp, fontWeight = FontWeight.SemiBold)
    }
}

/** Wybor godziny przez pola HH i MM (bez zewnetrznego pickera). */
@Composable
fun TimeField(
    label: String,
    minutes: Int,
    modifier: Modifier = Modifier,
    onChange: (Int) -> Unit
) {
    Column(modifier, verticalArrangement = Arrangement.spacedBy(4.dp)) {
        Text(label, fontSize = 8.sp, color = TextDim, fontWeight = FontWeight.Bold)
        Row(
            Modifier.fillMaxWidth().clip(RoundedCornerShape(12.dp))
                .background(Surface2).padding(horizontal = 10.dp, vertical = 6.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(6.dp)
        ) {
            Text(
                Logic.fmtHHMM(minutes), fontSize = 10.sp,
                color = TextHi, fontWeight = FontWeight.Bold,
                modifier = Modifier.weight(1f)
            )
            GhostButton("−15", onClick = { onChange(((minutes - 15) % 1440 + 1440) % 1440) })
            GhostButton("+15", onClick = { onChange((minutes + 15) % 1440) })
        }
    }
}
