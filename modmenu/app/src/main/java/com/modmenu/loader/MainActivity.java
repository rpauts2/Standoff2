package com.modmenu.loader;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.LinearGradient;
import android.graphics.Shader;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.view.animation.OvershootInterpolator;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends Activity {

    private TextView statusText;
    private TextView statusDot;
    private Button injectButton;
    private Button toggleButton;
    private ProgressBar progressBar;
    private LinearLayout featuresContainer;
    private Handler handler = new Handler(Looper.getMainLooper());
    private boolean injected = false;

    private static final String[] FEATURE_NAMES = {
        "Box ESP", "Snaplines", "Health Bar", "Name ESP",
        "Aimbot", "Triggerbot", "No Recoil", "Wallhack",
        "Chams", "Crosshair", "Speed Hack", "Infinite Ammo"
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        ScrollView scrollView = new ScrollView(this);
        scrollView.setFillViewport(true);

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setBackgroundColor(Color.parseColor("#0D1117"));
        root.setPadding(dp(24), dp(24), dp(24), dp(24));

        // Header
        LinearLayout header = new LinearLayout(this);
        header.setOrientation(LinearLayout.VERTICAL);
        header.setGravity(Gravity.CENTER);
        header.setPadding(0, dp(40), 0, dp(20));
        root.addView(header);

        // Logo circle
        FrameLayout logoContainer = new FrameLayout(this);
        LinearLayout.LayoutParams logoParams = new LinearLayout.LayoutParams(dp(80), dp(80));
        logoParams.gravity = Gravity.CENTER;
        logoParams.bottomMargin = dp(16);
        header.addView(logoContainer, logoParams);

        View logoCircle = new View(this);
        GradientDrawable logoBg = new GradientDrawable();
        logoBg.setShape(GradientDrawable.OVAL);
        int[] gradientColors = {Color.parseColor("#58A6FF"), Color.parseColor("#1F6FEB")};
        logoBg.setColors(gradientColors);
        logoCircle.setBackground(logoBg);
        logoContainer.addView(logoCircle);

        TextView logoText = new TextView(this);
        logoText.setText("S2");
        logoText.setTextColor(Color.WHITE);
        logoText.setTextSize(28);
        logoText.setTypeface(null, Typeface.BOLD);
        logoText.setGravity(Gravity.CENTER);
        FrameLayout.LayoutParams logoTextParams = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT);
        logoContainer.addView(logoText, logoTextParams);

        TextView title = new TextView(this);
        title.setText("SO2 MOD MENU");
        title.setTextColor(Color.WHITE);
        title.setTextSize(22);
        title.setTypeface(null, Typeface.BOLD);
        title.setGravity(Gravity.CENTER);
        title.setLetterSpacing(0.15f);
        header.addView(title);

        TextView subtitle = new TextView(this);
        subtitle.setText("v1.0 | Premium Edition");
        subtitle.setTextColor(Color.parseColor("#8B949E"));
        subtitle.setTextSize(13);
        subtitle.setGravity(Gravity.CENTER);
        subtitle.setPadding(0, dp(4), 0, 0);
        header.addView(subtitle);

        // Status card
        LinearLayout statusCard = createCard(root);
        statusCard.setOrientation(LinearLayout.HORIZONTAL);
        statusCard.setGravity(Gravity.CENTER_VERTICAL);
        statusCard.setPadding(dp(16), dp(14), dp(16), dp(14));

        statusDot = new TextView(this);
        statusDot.setText("\u25CF");
        statusDot.setTextSize(18);
        statusDot.setTextColor(Color.parseColor("#F85149"));
        statusDot.setPadding(0, 0, dp(10), 0);
        statusCard.addView(statusDot);

        statusText = new TextView(this);
        statusText.setText("Not Injected");
        statusText.setTextColor(Color.parseColor("#8B949E"));
        statusText.setTextSize(15);
        statusText.setTypeface(null, Typeface.BOLD);
        statusCard.addView(statusText);

        progressBar = new ProgressBar(this);
        progressBar.setIndeterminate(true);
        progressBar.setVisibility(View.GONE);
        LinearLayout.LayoutParams progressParams = new LinearLayout.LayoutParams(dp(24), dp(24));
        progressParams.gravity = Gravity.END;
        statusCard.addView(progressBar, progressParams);

        // Buttons
        LinearLayout btnContainer = new LinearLayout(this);
        btnContainer.setOrientation(LinearLayout.VERTICAL);
        btnContainer.setPadding(0, dp(12), 0, 0);
        root.addView(btnContainer);

        injectButton = createButton("INJECT", "#238636", "#2EA043", btnContainer);
        injectButton.setOnClickListener(v -> {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !Settings.canDrawOverlays(this)) {
                startActivity(new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                        Uri.parse("package:" + getPackageName())));
                return;
            }
            startInjection();
        });

        toggleButton = createButton("TOGGLE MENU", "#1F6FEB", "#388BFD", btnContainer);
        toggleButton.setEnabled(false);
        toggleButton.setAlpha(0.5f);
        toggleButton.setOnClickListener(v -> NativeLoader.toggleMenu());

        // Features section
        TextView featuresTitle = new TextView(this);
        featuresTitle.setText("ACTIVE FEATURES");
        featuresTitle.setTextColor(Color.parseColor("#8B949E"));
        featuresTitle.setTextSize(11);
        featuresTitle.setTypeface(null, Typeface.BOLD);
        featuresTitle.setLetterSpacing(0.1f);
        featuresTitle.setPadding(0, dp(20), 0, dp(10));
        root.addView(featuresTitle);

        featuresContainer = new LinearLayout(this);
        featuresContainer.setOrientation(LinearLayout.VERTICAL);
        root.addView(featuresContainer);

        for (String feature : FEATURE_NAMES) {
            addFeatureRow(feature);
        }

        // Footer
        TextView footer = new TextView(this);
        footer.setText("By Opencode | Educational Purpose Only");
        footer.setTextColor(Color.parseColor("#484F58"));
        footer.setTextSize(11);
        footer.setGravity(Gravity.CENTER);
        footer.setPadding(0, dp(24), 0, dp(12));
        root.addView(footer);

        scrollView.addView(root);
        setContentView(scrollView);

        createNotificationChannel();
    }

    private LinearLayout createCard(ViewGroup parent) {
        LinearLayout card = new LinearLayout(this);
        GradientDrawable cardBg = new GradientDrawable();
        cardBg.setCornerRadius(dp(12));
        cardBg.setColor(Color.parseColor("#161B22"));
        cardBg.setStroke(dp(1), Color.parseColor("#30363D"));
        card.setBackground(cardBg);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT);
        params.bottomMargin = dp(12);
        parent.addView(card, params);
        return card;
    }

    private Button createButton(String text, String colorNormal, String colorPressed, ViewGroup parent) {
        Button btn = new Button(this);
        btn.setText(text);
        btn.setTextColor(Color.WHITE);
        btn.setTextSize(14);
        btn.setTypeface(null, Typeface.BOLD);
        btn.setAllCaps(false);

        GradientDrawable btnBg = new GradientDrawable();
        btnBg.setCornerRadius(dp(10));
        btnBg.setColor(Color.parseColor(colorNormal));
        btn.setBackground(btnBg);

        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, dp(50));
        params.bottomMargin = dp(10);
        parent.addView(btn, params);
        return btn;
    }

    private void addFeatureRow(String name) {
        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);
        row.setPadding(dp(12), dp(10), dp(12), dp(10));

        GradientDrawable rowBg = new GradientDrawable();
        rowBg.setCornerRadius(dp(8));
        rowBg.setColor(Color.parseColor("#161B22"));
        rowBg.setStroke(dp(1), Color.parseColor("#21262D"));
        row.setBackground(rowBg);

        TextView check = new TextView(this);
        check.setText("\u2713");
        check.setTextColor(Color.parseColor("#3FB950"));
        check.setTextSize(14);
        check.setTypeface(null, Typeface.BOLD);
        check.setPadding(0, 0, dp(10), 0);
        row.addView(check);

        TextView nameView = new TextView(this);
        nameView.setText(name);
        nameView.setTextColor(Color.parseColor("#C9D1D9"));
        nameView.setTextSize(14);
        row.addView(nameView);

        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        params.bottomMargin = dp(6);
        featuresContainer.addView(row, params);
    }

    private void startInjection() {
        progressBar.setVisibility(View.VISIBLE);
        injectButton.setEnabled(false);
        statusText.setText("Injecting...");
        statusText.setTextColor(Color.parseColor("#D29922"));
        statusDot.setTextColor(Color.parseColor("#D29922"));

        handler.postDelayed(() -> {
            NativeLoader.loadNative();
            injected = true;

            progressBar.setVisibility(View.GONE);
            statusText.setText("Injected");
            statusText.setTextColor(Color.parseColor("#3FB950"));
            statusDot.setTextColor(Color.parseColor("#3FB950"));

            injectButton.setEnabled(false);
            injectButton.setAlpha(0.5f);
            toggleButton.setEnabled(true);
            toggleButton.setAlpha(1.0f);

            startForegroundService(new Intent(this, OverlayService.class));
            Toast.makeText(this, "Switch to Standoff 2", Toast.LENGTH_LONG).show();
            moveTaskToBack(true);
        }, 1500);
    }

    private void createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(
                    "modmenu", "Mod Menu",
                    NotificationManager.IMPORTANCE_LOW);
            channel.setDescription("Keeps mod menu running");
            getSystemService(NotificationManager.class).createNotificationChannel(channel);
        }
    }

    @Override
    public void onBackPressed() {
        moveTaskToBack(true);
    }

    private int dp(int value) {
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP,
                value, getResources().getDisplayMetrics());
    }
}
