package com.chenxf.opengles;

import android.Manifest;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.chenxf.opengles.audio.AudioCollector;

import static android.opengl.GLSurfaceView.RENDERMODE_CONTINUOUSLY;
import static com.chenxf.opengles.MyNativeRender.SAMPLE_TYPE;
import static com.chenxf.opengles.MyNativeRender.SAMPLE_TYPE_3D_MODEL;
import static com.chenxf.opengles.MyNativeRender.SAMPLE_TYPE_3D_MODEL_ANIM;
import static com.chenxf.opengles.MyNativeRender.SAMPLE_TYPE_TEXT;
import static com.chenxf.opengles.MyNativeRender.SAMPLE_TYPE_TRIANGLE;
import static com.chenxf.opengles.MyNativeRender.SAMPLE_TYPE_TEXT_ENGLISH;

public class MainActivity extends Activity implements AudioCollector.Callback, ViewTreeObserver.OnGlobalLayoutListener, SensorEventListener, View.OnClickListener, MyGLRender.FPSListener {
    private static final String TAG = "MainActivity";
    private static final String[] REQUEST_PERMISSIONS = {
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.READ_EXTERNAL_STORAGE,
    };
    private static final int PERMISSION_REQUEST_CODE = 1;

    private MyGLSurfaceView mGLSurfaceView;
    private ViewGroup mRootView;
    private int mSampleSelectedIndex = SAMPLE_TYPE_3D_MODEL - SAMPLE_TYPE;
    private AudioCollector mAudioCollector;
    private MyGLRender mGLRender = new MyGLRender();
    private SensorManager mSensorManager;
    private TextView mTextView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        findViewById(R.id.button_start_0).setOnClickListener(this);
        findViewById(R.id.button_start_1).setOnClickListener(this);
        findViewById(R.id.button_start_2).setOnClickListener(this);
        findViewById(R.id.button_start_3).setOnClickListener(this);
        findViewById(R.id.button_start_4).setOnClickListener(this);

        mTextView = findViewById(R.id.text_info);
        mRootView = (ViewGroup) findViewById(R.id.rootView);
        mRootView.getViewTreeObserver().addOnGlobalLayoutListener(this);
        mSensorManager = (SensorManager) getSystemService(SENSOR_SERVICE);
        mGLRender.init();
        mGLRender.setOnFPSListener(this);

    }

    @Override
    public void onGlobalLayout() {
        mRootView.getViewTreeObserver().removeOnGlobalLayoutListener(this);
        RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        lp.addRule(RelativeLayout.CENTER_IN_PARENT);
        mGLSurfaceView = new MyGLSurfaceView(this, mGLRender);
        mRootView.addView(mGLSurfaceView, 0, lp);
        mGLSurfaceView.setRenderMode(RENDERMODE_CONTINUOUSLY);
    }

    @Override
    protected void onResume() {
        super.onResume();
        mSensorManager.registerListener(this,
                mSensorManager.getDefaultSensor(Sensor.TYPE_GRAVITY),
                SensorManager.SENSOR_DELAY_FASTEST);
        if (!hasPermissionsGranted(REQUEST_PERMISSIONS)) {
            requestPermissions( REQUEST_PERMISSIONS, PERMISSION_REQUEST_CODE);
        }

        String fileDir = getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS).getAbsolutePath();
        CommonUtils.copyAssetsDirToSDCard(this, "avata1", fileDir + "/model");
        CommonUtils.copyAssetsDirToSDCard(this, "avata2", fileDir + "/model");
        CommonUtils.copyAssetsDirToSDCard(this, "vampire", fileDir + "/model");
        //font related
        CommonUtils.copyAssetsDirToSDCard(this, "fonts", fileDir);

    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        if (requestCode == PERMISSION_REQUEST_CODE) {
            if (!hasPermissionsGranted(REQUEST_PERMISSIONS)) {
                Toast.makeText(this, "We need the permission: WRITE_EXTERNAL_STORAGE", Toast.LENGTH_SHORT).show();
            }
        } else {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        mSensorManager.unregisterListener(this);
        if (mAudioCollector != null) {
            mAudioCollector.unInit();
            mAudioCollector = null;
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mGLRender.unInit();
        /*
        * Once the EGL context gets destroyed all the GL buffers etc will get destroyed with it,
        * so this is unnecessary.
        * */
    }

    @Override
    public void onAudioBufferCallback(short[] buffer) {
        Log.e(TAG, "onAudioBufferCallback() called with: buffer[0] = [" + buffer[0] + "]");
        mGLRender.setAudioData(buffer);
        //mGLSurfaceView.requestRender();
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
        switch (event.sensor.getType()) {
            case Sensor.TYPE_GRAVITY:
                Log.d(TAG, "onSensorChanged() called with TYPE_GRAVITY: [x,y,z] = [" + event.values[0] + ", " + event.values[1] + ", " + event.values[2] + "]");
                mGLRender.setGravityXY(event.values[0], event.values[1]);
                break;
        }

    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {

    }

    protected boolean hasPermissionsGranted(String[] permissions) {
        for (String permission : permissions) {
            if (checkSelfPermission(permission)
                    != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }
        return true;
    }

    @Override
    public void onClick(View view) {
        if(view.getId() == R.id.button_start_0) {
            Log.e(TAG, "start to render triangle");
            mGLRender.setParamsInt(SAMPLE_TYPE, SAMPLE_TYPE_TRIANGLE, 0);
        } else if(view.getId() == R.id.button_start_1) {
            Log.e(TAG, "start to render 3D model");
            mGLRender.setParamsInt(SAMPLE_TYPE, SAMPLE_TYPE_3D_MODEL, 0);
        } else if(view.getId() == R.id.button_start_2) {
            Log.e(TAG, "start to render 3D anmi model");
            mGLRender.setParamsInt(SAMPLE_TYPE, SAMPLE_TYPE_3D_MODEL_ANIM, 0);
        } else if(view.getId() == R.id.button_start_3) {
            Log.e(TAG, "start to render text");
            mGLRender.setParamsInt(SAMPLE_TYPE, SAMPLE_TYPE_TEXT, 0);
        } else if(view.getId() == R.id.button_start_4) {
            Log.e(TAG, "start to render text");
            mGLRender.setParamsInt(SAMPLE_TYPE, SAMPLE_TYPE_TEXT_ENGLISH, 0);
        }
    }

    @Override
    public void onFpsUpdate(final int fps) {
        mTextView.post(new Runnable() {
            @Override
            public void run() {
                mTextView.setText("fps: " + fps);
            }
        });
    }
}
