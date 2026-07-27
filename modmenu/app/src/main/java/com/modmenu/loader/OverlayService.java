package com.modmenu.loader;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.drawable.GradientDrawable;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.animation.OvershootInterpolator;
import android.widget.FrameLayout;
import android.widget.TextView;

public class OverlayService extends Service {

    private WindowManager wm;
    private View floatingBtn;
    private View menuIndicator;
    private boolean menuVisible = false;
    private Handler handler = new Handler(Looper.getMainLooper());

    @Override
    public void onCreate() {
        super.onCreate();
        wm = (WindowManager) getSystemService(WINDOW_SERVICE);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        startForeground(1, createNotification());
        createFloatingButton();
        return START_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) { return null; }

    private void createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel ch = new NotificationChannel(
                    "modmenu", "Mod Menu",
                    NotificationManager.IMPORTANCE_LOW);
            ch.setDescription("Running");
            getSystemService(NotificationManager.class).createNotificationChannel(ch);
        }
    }

    private Notification createNotification() {
        createNotificationChannel();
        Intent i = new Intent(this, MainActivity.class);
        PendingIntent pi = PendingIntent.getActivity(this, 0, i, PendingIntent.FLAG_IMMUTABLE);

        Notification.Builder b;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            b = new Notification.Builder(this, "modmenu");
        } else {
            b = new Notification.Builder(this);
        }
        return b.setContentTitle("SO2 Mod Menu")
                .setContentText("Active - Tap to open")
                .setSmallIcon(android.R.drawable.ic_dialog_info)
                .setContentIntent(pi)
                .setOngoing(true)
                .build();
    }

    private void createFloatingButton() {
        int type = Build.VERSION.SDK_INT >= Build.VERSION_CODES.O
                ? WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY
                : WindowManager.LayoutParams.TYPE_PHONE;

        WindowManager.LayoutParams params = new WindowManager.LayoutParams(
                dp(52), dp(52), type,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
                PixelFormat.TRANSLUCENT);
        params.gravity = Gravity.TOP | Gravity.START;
        params.x = dp(16);
        params.y = dp(200);

        // Main button
        floatingBtn = new View(this);
        GradientDrawable bg = new GradientDrawable();
        bg.setShape(GradientDrawable.OVAL);
        bg.setColor(Color.parseColor("#1F6FEB"));
        bg.setStroke(dp(2), Color.parseColor("#58A6FF"));
        floatingBtn.setBackground(bg);

        // Inner icon
        TextView icon = new TextView(this);
        icon.setText("M");
        icon.setTextColor(Color.WHITE);
        icon.setTextSize(18);
        icon.setGravity(Gravity.CENTER);
        FrameLayout wrapper = new FrameLayout(this);
        wrapper.addView(icon, new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT));
        wrapper.addView(floatingBtn);

        // Drag state
        final float[] touchX = new float[2];
        final float[] touchY = new float[2];
        final int[] startX = new int[1];
        final int[] startY = new int[1];
        final boolean[] moved = new boolean[1];
        final long[] downTime = new long[1];

        wrapper.setOnTouchListener((v, event) -> {
            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    touchX[0] = event.getRawX();
                    touchY[0] = event.getRawY();
                    startX[0] = params.x;
                    startY[0] = params.y;
                    moved[0] = false;
                    downTime[0] = System.currentTimeMillis();
                    wrapper.animate().scaleX(0.9f).scaleY(0.9f).setDuration(100).start();
                    return true;

                case MotionEvent.ACTION_MOVE:
                    float dx = event.getRawX() - touchX[0];
                    float dy = event.getRawY() - touchY[0];
                    if (Math.abs(dx) > 5 || Math.abs(dy) > 5) {
                        moved[0] = true;
                        params.x = startX[0] + (int) dx;
                        params.y = startY[0] + (int) dy;
                        wm.updateViewLayout(wrapper, params);
                    }
                    return true;

                case MotionEvent.ACTION_UP:
                    wrapper.animate().scaleX(1f).scaleY(1f).setDuration(150)
                            .setInterpolator(new OvershootInterpolator()).start();
                    if (!moved[0] && System.currentTimeMillis() - downTime[0] < 300) {
                        toggleMenu();
                    }
                    return true;
            }
            return false;
        });

        wm.addView(wrapper, params);

        // Pulse animation
        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                if (floatingBtn != null) {
                    floatingBtn.animate().scaleX(1.1f).scaleY(1.1f).setDuration(800)
                            .withEndAction(() -> {
                                if (floatingBtn != null) {
                                    floatingBtn.animate().scaleX(1f).scaleY(1f)
                                            .setDuration(800).start();
                                }
                            }).start();
                }
                handler.postDelayed(this, 2000);
            }
        }, 1000);
    }

    private void toggleMenu() {
        menuVisible = !menuVisible;
        NativeLoader.toggleMenu();

        GradientDrawable bg = (GradientDrawable) floatingBtn.getBackground();
        if (menuVisible) {
            bg.setColor(Color.parseColor("#F85149"));
            bg.setStroke(dp(2), Color.parseColor("#FF7B72"));
        } else {
            bg.setColor(Color.parseColor("#1F6FEB"));
            bg.setStroke(dp(2), Color.parseColor("#58A6FF"));
        }
    }

    private int dp(int v) {
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP,
                v, getResources().getDisplayMetrics());
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (floatingBtn != null) {
            try { wm.removeView(floatingBtn.getParent() instanceof View ? (View) floatingBtn.getParent() : floatingBtn); } catch (Exception e) {}
        }
    }
}
