package oaknut;

import android.app.Application;
import android.content.SharedPreferences;
import android.content.res.AssetManager;
import android.preference.PreferenceManager;
import android.util.Log;

import java.nio.charset.Charset;
import java.util.UUID;



public class App extends Application {

    // TODO: Move all this into MainActivity

    public static App app;
    static SharedPreferences prefs;
    public static final Charset UTF_8 = Charset.forName("UTF-8");

    public App() {
        app = this;
    }

    @Override
    public void onCreate() {
      super.onCreate();
      prefs = PreferenceManager.getDefaultSharedPreferences(this);
    }

    public static AssetManager getAssetManager() {
        return app.getAssets();
    }
/*
    public static byte[] loadAsset(String assetPath) {
        try {
            InputStream inputStream = app.getAssets().open(assetPath);
            byte[] buffer = new byte[8192];
            int bytesRead;
            ByteArrayOutputStream output = new ByteArrayOutputStream();
            while ((bytesRead = inputStream.read(buffer)) != -1) {
                output.write(buffer, 0, bytesRead);
            }
            inputStream.close();
            return output.toByteArray();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        return null;
    }
*/
    public static byte[] resolvePath(byte[] aPath) {
        String path = new String(aPath, UTF_8);

        //assets/...
        if (path.startsWith("//assets/")) {
            // do nothing, this is handled in a hideous C hack
        }

        //data/... : /data/data/package/files
        else if (path.startsWith("//data/")) {
            path = app.getFilesDir().getAbsolutePath() + path.substring(7);
        }

        //cache/... : /data/data/package/cache
        else if (path.startsWith("//cache/")) {
            path = app.getCacheDir().getAbsolutePath() + path.substring(8);
        }

        //docs/... : ~/Documents/...
        else if (path.startsWith("//docs/")) {
            path = app.getFilesDir().getAbsolutePath() + path.substring(7);
        }

        //tmp/... : /data/data/package/
        else if (path.startsWith("//tmp/")) {
            path = app.getFilesDir().getParent() + "/tmp/" + path.substring(7);
        }

        // :-(
        else {
            Log.w("Oaknut", "Unknown path: " + path);
        }

        return path.getBytes(UTF_8);
    }
    public static byte[] getCurrentCountryCode() {
        return app.getResources().getConfiguration().locale.getCountry().getBytes(UTF_8);
    }

    public static int getPrefsInt(byte[] key, int defaultVal) {
        return prefs.getInt(new String(key, UTF_8), defaultVal);
    }
    public static void setPrefsInt(byte[] key, int val) {
        prefs.edit().putInt(new String(key, UTF_8),val).commit();
    }

    public static byte[] getPrefsString(byte[] key, byte[] defaultVal) {
        if (defaultVal == null) defaultVal=new byte[] {};
        String s = prefs.getString(new String(key, UTF_8), new String(defaultVal, UTF_8));
        return s.getBytes(UTF_8);
    }
    public static void setPrefsString(byte[] key, byte[] val) {
        prefs.edit().putString(new String(key, UTF_8), new String(val, UTF_8)).commit();
    }

    public static byte[] getPrefsByteArray(String key, byte[] defaultVal) {
        String s = prefs.getString(key, null);
        if (s == null) {
            return defaultVal;
        }
        return android.util.Base64.decode(s, 0);
    }
    public static void setPrefsByteArray(String key, byte[] val) {
        prefs.edit().putString(key, android.util.Base64.encodeToString(val, 0)).commit();
    }

    public static String createUUID() {
        return UUID.randomUUID().toString();
    }

}
