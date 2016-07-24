//
//  PictureDownloader.java
//  SpaceExplorer
//
//  Created by João Baptista on 20/09/15.
//
//

package joaobapt;

import java.lang.*;
import java.net.*;
import java.io.*;
import java.nio.ByteBuffer;
import android.os.*;

public class PictureDownloader extends AsyncTask<URL, Void, ByteBuffer>
{
    public ByteBuffer callback;
    public String errorMessage;
    
    public PictureDownloader(ByteBuffer callback)
    {
        super();
        this.callback = callback;
    }
    
    @Override
    protected ByteBuffer doInBackground(URL... args)
    {
        URL picture = args[0];
        
        InputStream in = null;
        
        try
        {
            in = picture.openStream();
            ByteArrayOutputStream stream = new ByteArrayOutputStream(2048);
            
            int curByte;
            while ((curByte = in.read()) != -1)
                stream.write(curByte);
            
            errorMessage = "";
            return ByteBuffer.wrap(stream.toByteArray());
        }
        catch (IOException exc)
        {
            errorMessage = exc.getLocalizedMessage();
            return ByteBuffer.allocateDirect(0);
        }
        finally
        {
            try { in.close(); }
            catch (Exception exc) {}
        }
    }
    
    @Override
    protected native void onPostExecute(ByteBuffer result);
}