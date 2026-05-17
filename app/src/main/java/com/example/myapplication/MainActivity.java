package com.example.myapplication;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.media.AudioAttributes;
import android.media.SoundPool;
import android.os.Build;
import android.os.Bundle;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.os.VibratorManager;
import android.view.View;
import com.google.androidgamesdk.GameActivity;
import java.io.IOException;

public class MainActivity extends GameActivity {
    static {
        System.loadLibrary("myapplication");
    }

    private SoundPool soundPool;
    private int soundPistol, soundShotgun, soundBazooka, soundDamage, soundDeath, soundWin;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        initSounds();
    }

    private void initSounds() {
        AudioAttributes attrs = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build();
        soundPool = new SoundPool.Builder()
                .setMaxStreams(10)
                .setAudioAttributes(attrs)
                .build();

        soundPistol = loadSound("pistol.wav");
        soundShotgun = loadSound("shotgun.wav");
        soundBazooka = loadSound("bazooka.wav");
        soundDamage = loadSound("damage.mp3");
        soundDeath = loadSound("death.mp3");
        soundWin = loadSound("win.wav"); // Corrigido para .wav
    }

    private int loadSound(String fileName) {
        try {
            AssetFileDescriptor afd = getAssets().openFd(fileName);
            return soundPool.load(afd, 1);
        } catch (IOException e) {
            return -1;
        }
    }

    public void playPistol() { if (soundPistol != -1) soundPool.play(soundPistol, 0.3f, 0.3f, 1, 0, 1.0f); }
    public void playShotgun() { if (soundShotgun != -1) soundPool.play(soundShotgun, 0.4f, 0.4f, 1, 0, 1.0f); }
    public void playBazooka() { if (soundBazooka != -1) soundPool.play(soundBazooka, 0.5f, 0.5f, 1, 0, 1.0f); }
    public void playDamage() { if (soundDamage != -1) soundPool.play(soundDamage, 0.3f, 0.3f, 1, 0, 1.0f); }
    public void playDeath() { if (soundDeath != -1) soundPool.play(soundDeath, 0.5f, 0.5f, 1, 0, 1.0f); }
    public void playWin() { if (soundWin != -1) soundPool.play(soundWin, 0.5f, 0.5f, 1, 0, 1.0f); }

    private Vibrator getVibrator() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            VibratorManager vm = (VibratorManager) getSystemService(Context.VIBRATOR_MANAGER_SERVICE);
            return vm.getDefaultVibrator();
        } else {
            return (Vibrator) getSystemService(Context.VIBRATOR_SERVICE);
        }
    }

    public void vibrateLight() {
        Vibrator v = getVibrator();
        if (v != null) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) v.vibrate(VibrationEffect.createOneShot(50, 100));
            else v.vibrate(50);
        }
    }

    public void vibrateHeavy() {
        Vibrator v = getVibrator();
        if (v != null) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) v.vibrate(VibrationEffect.createOneShot(500, 255));
            else v.vibrate(500);
        }
    }

    public void vibrateLevelComplete() {
        Vibrator v = getVibrator();
        if (v != null) {
            long[] pattern = {0, 100, 100, 100, 100, 300};
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) v.vibrate(VibrationEffect.createWaveform(pattern, -1));
            else v.vibrate(pattern, -1);
        }
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) hideSystemUi();
    }

    private void hideSystemUi() {
        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY | View.SYSTEM_UI_FLAG_LAYOUT_STABLE |
                View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_FULLSCREEN
        );
    }
}
