/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.coconutadventure.android;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.NativeActivity;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.WindowManager;

import com.android.gl2jni.Keep;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;


public class GL2JNIActivity extends NativeActivity {
	public static final String SCRIPT_LOCATION = "GL2JNIActivity.SCRIPT_LOCATION";
	private String scriptLocation;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		amgr = getAssets();
		Bundle extras = getIntent().getExtras();

		if (extras != null && extras.containsKey(SCRIPT_LOCATION)) {
			scriptLocation = extras.getString(SCRIPT_LOCATION);
		} else {
			scriptLocation = "/scripts/game.lua";
		}

		super.onCreate(savedInstanceState);
	}

	private AssetManager amgr;

	@Keep
	public Bitmap open(String path)
	{
		try
		{
			Log.e("GL2JNIActivity","Loading path: "+path);
			return BitmapFactory.decodeStream(amgr.open(path));
		}
		catch (Exception e) {
			Log.e("COCONUT","GL2JNIActivity",e);
		}
		return null;
	}

	@Keep
	public AssetManager getAssetManager() {
		return getAssets();
	}

	@Keep
	public int getWidth(Bitmap bmp) { return bmp.getWidth(); }

	@Keep
	public int getHeight(Bitmap bmp) { return bmp.getHeight(); }

	@Keep
	public void getPixels(Bitmap bmp, int[] pixels)
	{
		int w = bmp.getWidth();
		int h = bmp.getHeight();
		bmp.getPixels(pixels, 0, w, 0, 0, w, h);
	}

	@Keep
	public int getPixelFormat(Bitmap bmp) {
		return bmp.getConfig().ordinal();
	}

	@Keep
	public void writeBitmap(Bitmap bmp) {
		FileOutputStream out = null;
		try {
			String location = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES)+ "/" + "Atlas.png";
			Log.e("GLOUT","BITMAP LOCATION: "+location);
			out = new FileOutputStream(location);
			bmp.compress(Bitmap.CompressFormat.PNG, 100, out); // bmp is your Bitmap instance
			// PNG is a lossless format, the compression factor (100) is ignored
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			try {
				if (out != null) {
					out.close();
				}
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}

	@Keep
	public String getLuaScript() {
		return scriptLocation;
	}

	@Keep
	public void close(Bitmap bmp)
	{
		bmp.recycle();
	}
}
