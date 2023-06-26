package com.example.androidex;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import java.util.Date;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.graphics.YuvImage;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.view.View.OnClickListener;
import android.widget.ImageView;
import android.text.format.DateFormat;
import android.util.Log;

public class MainActivity extends Activity implements OnTouchListener, OnClickListener {
	private static final String TAG = "WebCam";
	private native int moveMotorRight();
	private native int moveMotorLeft();
	private native int stopMotor();
	private native byte[] capture();
	ImageView webCam;
	Bitmap bitmap = null;
	byte[] source = null;
	
	Thread thread;
	static{
		System.loadLibrary("motor_jni");
	};
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		Log.d(TAG, "Start Application");
		setContentView(R.layout.fragment_main);
		View leftButton = findViewById(R.id.left_button);
		leftButton.setOnTouchListener(this);     
		View rightButton = findViewById(R.id.right_button);
		rightButton.setOnTouchListener(this);
		View shootButton = findViewById(R.id.shoot_button);
		shootButton.setOnClickListener(this);
		webCam = (ImageView) findViewById(R.id.web_cam);
		
		if(thread == null) {
			thread = new Thread(new WebCamImage());
		}
		thread.start();
		
	}
	
	@Override
	public boolean onTouch(View v, MotionEvent event) {
		// TODO Auto-generated method stub
		switch(v.getId()) {
			case R.id.right_button :
				switch(event.getAction()) {
			        case MotionEvent.ACTION_DOWN:
			            // PRESSED
			        	Log.d(TAG, "Pressed Right");
			        	Log.d(TAG, String.valueOf(moveMotorRight()));
			            return true; // if you want to handle the touch event
			        case MotionEvent.ACTION_UP:
			            // RELEASED
			        	Log.d(TAG, "Released Right");
			        	Log.d(TAG, String.valueOf(stopMotor()));
			        	return true; // if you want to handle the touch event
				}
			case R.id.left_button :
				switch(event.getAction()) {
			        case MotionEvent.ACTION_DOWN:
			            // PRESSED
			        	Log.d(TAG, "Pressed Left");
			        	Log.d(TAG, String.valueOf(moveMotorLeft()));
			            return true; // if you want to handle the touch event
			        case MotionEvent.ACTION_UP:
			            // RELEASED
			        	Log.d(TAG, "Released Left");
			        	Log.d(TAG, String.valueOf(stopMotor()));
			            return true; // if you want to handle the touch event
				}
		}
		return false;
	}

	public void onClick(View v) {
		// TODO Auto-generated method stub
		switch(v.getId()){
			case R.id.shoot_button:
				SimpleDateFormat formatter = new SimpleDateFormat("yyyy-MM-dd-hh-mm-ss");
				Log.d(TAG, formatter.format(new Date()));
				OutputStream out = null;
			try {
				File file = new File("/sdcard/DCIM", formatter.format(new Date()) + ".png");
				file.createNewFile();
				out = new FileOutputStream(file);
				Context context = this.getBaseContext();
				context.sendBroadcast(new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE, Uri.fromFile(file)));
				bitmap.compress(Bitmap.CompressFormat.PNG,  80, out);
			} catch (FileNotFoundException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}	
		}
	}
	
	class WebCamImage implements Runnable{
		Handler handler = new Handler();
		boolean hasImageAlready = (webCam.getDrawable() != null);
		@Override
		public void run() {
			// TODO Auto-generated method stub
			while(true) {
				// get Image
				Log.d(TAG, "Get Image");
				source = capture();
				if(source != null) {
					Log.d(TAG, "Image Not NULL : ");
					// decode Image 
					ByteArrayOutputStream out = new ByteArrayOutputStream();
					YuvImage yuvImage = new YuvImage(source, ImageFormat.YUY2, 640, 480, null);
					yuvImage.compressToJpeg(new Rect(0, 0, 480, 360), 100, out);
					byte[] imageBytes = out.toByteArray();
					
					bitmap = BitmapFactory.decodeByteArray(imageBytes, 0, imageBytes.length);				
					Log.d(TAG, bitmap.getConfig().name());
		
					if(bitmap != null) {
						Log.d(TAG, "Bitmap Not NULL");
						runOnUiThread(new Runnable() {
							@Override
							public void run() {
								// TODO Auto-generated method stub
								webCam.setImageBitmap(bitmap);
								Log.d(TAG, "Set Image");
							}
						});
					}
				}
			}
		}
		
	}
}
