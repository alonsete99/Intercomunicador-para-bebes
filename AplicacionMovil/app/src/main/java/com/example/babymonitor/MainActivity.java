package com.example.babymonitor;


import java.io.IOException;
import java.io.InputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import androidx.appcompat.app.AppCompatActivity;
import android.widget.TextView;
import android.widget.Toast;


public class MainActivity extends AppCompatActivity {


    private Button startButton, stopButton, receiveButton;

    /****************SOCKET PARAMETERS****************/
    InetAddress addr = InetAddress.getByName("192.168.4.1");
    int sendingPort = 8080;
    int receivingPort = 8081;
    DatagramSocket sendingSocket = new DatagramSocket(sendingPort);
    DatagramSocket receivingSocket = new DatagramSocket(receivingPort);

    TextView text;
    private int SAMPLE_RATE = 16000;    //SAMPLE RATE FOR SOUND DATA
    private boolean statusSend = false;
    private boolean statusReciv = false;



    public MainActivity() throws UnknownHostException, SocketException {
        Log.d("VS", "Socket Creation Failed!");
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        startButton = (Button) findViewById(R.id.start_button);
        stopButton = (Button) findViewById(R.id.stop_button);
        receiveButton = (Button) findViewById(R.id.receive_button);
        startButton.setOnClickListener(startListener);
        stopButton.setOnClickListener(stopListener);
        receiveButton.setOnClickListener(startReceiving);
    }

    private final OnClickListener stopListener = new OnClickListener() {
        @Override
        public void onClick(View arg0) {
            Toast toast1 = Toast.makeText(getApplicationContext(), "Finalizando tareas...", Toast.LENGTH_SHORT);
            toast1.setGravity(Gravity.CENTER, 5, 8);
            toast1.show();
            statusReciv = false;
            statusSend = false;
            finishTask();
            Log.d("VS", "Recorder released");
        }

    };

    private final OnClickListener startListener = new OnClickListener() {

        @Override
        public void onClick(View arg0) {
            Toast toast2 = Toast.makeText(getApplicationContext(), "Reproduciendo nana...", Toast.LENGTH_SHORT);
            toast2.setGravity(Gravity.CENTER, 5, 8);
            toast2.show();
            statusSend = true;
            startStreaming();
            Log.d("VS", "INICIO ENVIO DE SONIDO");
        }

    };

    private final OnClickListener startReceiving = new OnClickListener() {

        @Override
        public void onClick(View arg0) {
            Toast toast3 = Toast.makeText(getApplicationContext(), "Recibiendo datos...", Toast.LENGTH_SHORT);
            toast3.setGravity(Gravity.CENTER, 5, 8);
            toast3.show();
            statusReciv = true;
            receiveSound();
            Log.d("VS", "INICIO RECEPCION DE SONIDO");
        }

    };

    public void finishTask(){
        Thread finishThread = new Thread(new Runnable() {
            @Override
            public void run() {
                String mens = "FINALIZA";
                DatagramPacket message = new DatagramPacket(mens.getBytes(), mens.length(), addr, sendingPort);
                try {
                    Log.d("VS", "FINALIZADO...");
                    Thread.sleep(2000);
                    sendingSocket.send(message);


                } catch (IOException | InterruptedException e) {
                    e.printStackTrace();
                }
            }
        });
        finishThread.start();
    }

    public void receiveSound() {
        Thread receiveThread = new Thread(new Runnable() {
            private Object sound;

            @Override
            public void run() {
                AudioTrack audio;
                try {

                    String mens = "ENVIA";
                    DatagramPacket message = new DatagramPacket(mens.getBytes(), mens.length(), addr, sendingPort);
                    sendingSocket.send(message);
                    byte[] buffer = new byte[1024];
                    audio = new AudioTrack(AudioManager.STREAM_MUSIC, SAMPLE_RATE, AudioFormat.CHANNEL_OUT_DEFAULT, AudioFormat.ENCODING_PCM_16BIT, buffer.length, AudioTrack.MODE_STREAM);

                    DatagramPacket packet = new DatagramPacket(buffer, buffer.length, addr, receivingPort);

                    audio.play();
                    while (statusReciv) {

                        receivingSocket.receive(packet);
                        audio.write(buffer, 0, buffer.length);


                    }
                    audio.stop();
                } catch (Exception e) {
                    System.err.println(e);
                    e.printStackTrace();
                }

            }
        });
        receiveThread.start();
    }


    public void startStreaming() {


        Thread streamThread = new Thread(new Runnable() {

            @Override
            public void run() {
                try {
                    byte[] buff=new byte[1024];
                    String mens = "RECIBE";
                    DatagramPacket message = new DatagramPacket(mens.getBytes(), mens.length(), addr, sendingPort);
                    sendingSocket.send(message);
                    DatagramPacket packet;
                    Log.d("VS", "Socket Created");
                    InputStream rawData = getAssets().open("nana.wav");

                    while (statusSend) {

                        int bytes_read=rawData.read(buff, 0, buff.length);
                        packet = new DatagramPacket(buff, bytes_read, addr, sendingPort);
                        sendingSocket.send(packet);
                        Thread.sleep(58);

                    }
                    rawData.close();


                } catch (IOException | InterruptedException e) {
                    e.printStackTrace();
                }
            }

        });
        streamThread.start();
    }

}