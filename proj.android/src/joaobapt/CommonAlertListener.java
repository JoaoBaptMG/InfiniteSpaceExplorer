//
//  CommonAlertListener.java
//  SpaceExplorer
//
//  Created by João Baptista on 28/02/15.
//
//

package joaobapt;

import java.lang.*;
import java.nio.ByteBuffer;
import android.app.*;
import android.content.*;

public class CommonAlertListener implements DialogInterface.OnClickListener
{
    public ByteBuffer confirmCallback, cancelCallback;
    
    @Override
    public native void onClick(DialogInterface dialog, int which);
    
    public CommonAlertListener()
    {
        
    }
    
    public void presentDialog(final Activity activity, final String message, final String title, final String confirmCaption, final String cancelCaption, ByteBuffer confirmCallback, ByteBuffer cancelCallback)
    {
        this.confirmCallback = confirmCallback;
        this.cancelCallback = cancelCallback;
        
        final AlertDialog.Builder builder = new AlertDialog.Builder(activity);
        builder.setMessage(message).setTitle(title).setPositiveButton(confirmCaption, this).setNegativeButton(cancelCaption, this);
        
        activity.runOnUiThread(new Runnable()
        {
            public void run()
            {
                AlertDialog dialog = builder.create();
                dialog.show();
            }
        });
    }
}