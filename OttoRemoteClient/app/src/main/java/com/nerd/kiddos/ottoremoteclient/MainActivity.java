package com.nerd.kiddos.ottoremoteclient;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.LinkedList;
import java.util.Set;
import java.util.Queue;
import java.util.UUID;

public class MainActivity extends Activity {
    public static final String TAG = "MainActivity";
    private static final int RESET_CODE = 0;
    private static final int STOP_CODE = 1;
    private Communicator communicator;
    private Queue<Integer> command;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        command = new LinkedList<>();

        int[] ids = {R.id.btnForward, R.id.btnBackward, R.id.btnLeft, R.id.btnRight};
        for (int i = 0; i < ids.length; ++i) {
            Button button = findViewById(ids[i]);
            button.setOnTouchListener(new ButtonHandler(i + 2));
        }

        communicator = new Communicator();
        if (communicator.checkValidAdapter()) {
            if (communicator.checkAdpaterEnable()) {
                communicator.init();
                communicator.start();
            } else {
                Log.e(TAG, "fail to initialize communicator");
            }
        } else {
            Toast.makeText(MainActivity.this, "No Valid Bluetooth adapter found", Toast.LENGTH_SHORT).show();
        }
    }

    @Override
    protected void onActivityResult (int requestCode, int resultCode, Intent data) {
        if (resultCode == RESULT_OK) {
            communicator.init();
            communicator.start();
        }
    }


    private class Communicator extends Thread {
        public static final int REQUEST_ENABLE_BT = 1;
        private BluetoothAdapter bluetoothAdapter;
        private BluetoothSocket bluetoothSocket;
        private UUID uuid;
        private boolean running;

        public boolean checkValidAdapter() {
            bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
            if (bluetoothAdapter == null) {
                return false;
            }
            return true;
        }

        public boolean checkAdpaterEnable() {
            if (!bluetoothAdapter.isEnabled()) {
                Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
                return false;
            }
            return true;
        }

        public void init() {
            Set<BluetoothDevice> pairedDevices = bluetoothAdapter.getBondedDevices();
            if (pairedDevices.size() > 0) {
                // There are paired devices. Get the name and address of each paired device.
                for (BluetoothDevice device : pairedDevices) {
                    String deviceName = device.getName();
                    String deviceHardwareAddress = device.getAddress(); // MAC address

                    Log.i(TAG, "device name: " + deviceName);
                    Log.i(TAG, "device hardware address: " + deviceHardwareAddress);

                    if (deviceName.startsWith("HC-")) {
                        try {
                            uuid = UUID.fromString("00001101-0000-1000-8000-00805f9b34fb");
                            bluetoothSocket = device.createRfcommSocketToServiceRecord(uuid);
                        } catch (IOException e) {
                            Log.e(TAG, "Socket's create() method failed", e);
                        }
                        Log.i(TAG, "socket create to connect " + deviceName);
                        break;
                    }
                }
            }
        }

        @Override
        public void run() {
            bluetoothAdapter.cancelDiscovery();
            Log.i(TAG, "connecting to bluetooth");
            try {
                // Connect to the remote device through the socket. This call blocks
                // until it succeeds or throws an exception.
                bluetoothSocket.connect();
            } catch (IOException connectException) {
                // Unable to connect; close the socket and return.
                try {
                    bluetoothSocket.close();
                } catch (IOException closeException) {
                    Log.e(TAG, "Could not close the client socket", closeException);
                }
                return;
            }
            Log.i(TAG, "client socket opened");

            handleConnection();

            try {
                if (bluetoothSocket != null) {
                    bluetoothSocket.close();
                }
            } catch (IOException exception) {
                Log.e(TAG, exception.getMessage());
            }
            Log.i(TAG, "client socket closed");
        }

        public void close() {
            running = false;
        }

        public boolean isRunning() {
            return running;
        }

        private void handleConnection() {
            running = true;
            try {
                OutputStream outputStream = bluetoothSocket.getOutputStream();

                while (running) {
                    if (command.size() > 0) {
                        int c = command.remove();
                        outputStream.write(c);
                    }
                    try {
                        Thread.sleep(66);
                    } catch (InterruptedException exception) {
                        Log.e(TAG, exception.getMessage());
                    }
                }

                // before closing, send a reset signal
                outputStream.write(RESET_CODE);
            } catch (IOException exception) {
                Log.e(TAG, exception.getMessage());
            }
        }
    }

    @Override
    protected void onDestroy() {
        if (communicator != null) {
            communicator.close();
        }
        super.onDestroy();
    }

    private class ButtonHandler implements View.OnTouchListener {
        private int code;

        public ButtonHandler(int code) {
            this.code = code;
        }

        @Override
        public boolean onTouch(View view, MotionEvent motionEvent) {
            if (motionEvent.getAction() == MotionEvent.ACTION_DOWN) {
                if (communicator != null && communicator.isRunning()) {
                    command.add(code);
                    Log.d(TAG, "Command Code: " + code);
                }
            } else if (motionEvent.getAction() == MotionEvent.ACTION_UP) {
                if (communicator != null && communicator.isRunning()) {
                    command.add(STOP_CODE);
                    Log.d(TAG, "Stop Code: " + STOP_CODE);
                }
            }
            return false;
        }
    }
}
