package com.zxj.zplayer;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;
import android.widget.Toast;

import com.zxj.zplayer.databinding.ActivityMainBinding;

import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity {

    private ActivityMainBinding binding;
    private int PERMISSION_REQUEST = 0x1001;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        checkPermission();

        ZPlayer mPlayer = new ZPlayer();
        getLifecycle().addObserver(mPlayer);
        mPlayer.setDataSource("/sdcard/demo.mp4");

    }

    private void checkPermission() {
        if(ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) !=
                PackageManager.PERMISSION_GRANTED){
            Log.e("zuo","无权限，去申请权限");
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, PERMISSION_REQUEST);
        }else {
            Log.e("zuo","有权限");
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if(requestCode == PERMISSION_REQUEST){
            Log.e("zuo","申请到权限"+grantResults.length);
            if (grantResults[0] == PackageManager.PERMISSION_GRANTED){
                Toast.makeText(this,"已申请权限",Toast.LENGTH_SHORT).show();
            }else {
                Toast.makeText(this,"申请权限失败",Toast.LENGTH_SHORT).show();
            }
        }
    }
}