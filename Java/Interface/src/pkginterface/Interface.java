/*
En liten tribute till rxtx-biblioteket: RXTX binary builds provided as a courtesy of Mfizz Inc. (http://mfizz.com/).
Please see http://mfizz.com/oss/rxtx-for-java for more information.
*/
package pkginterface;

import gnu.io.CommPortIdentifier;
import gnu.io.SerialPort;
import gnu.io.SerialPortEvent;
import gnu.io.SerialPortEventListener;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Enumeration;
import java.util.Scanner;

public class Interface implements SerialPortEventListener{

    private SerialPort seriellport;
    private CommPortIdentifier comport = null;
    private static final int baud = 115200;
    private String portnamn = "Ingenting";
    private int svar;
    private InputStream indata;
    private OutputStream utdata;
    Scanner instruktion = new Scanner(System.in);
    private int[] dataBuffer = {0,0,0}; // Lagrar tre senaste hämtade bytes
    private int bufferCounter = 0; // Håller koll på var i buffern vi är
    private int combinedData = 0; // De två byten data, översatta till ett tal
   
    private void hittaport()
    {
        System.out.println("God dag. Vilken seriell port önskas anslutas till?");
        Scanner portval = new Scanner(System.in);
        while(true){
        portnamn = portval.next().toUpperCase();
        
        CommPortIdentifier port; //Sätt nuvarande port till null
        Enumeration allaportar = CommPortIdentifier.getPortIdentifiers();
        while(allaportar.hasMoreElements())
        {
            port = (CommPortIdentifier)allaportar.nextElement();
            if(port.getName().equals(portnamn)){                   //Gå igenom alla tillgängliga portar tills rätt port hittats!
                comport = port;
                break;
            }
        }
        if(comport != null)
        {
            System.out.println(portnamn + " hittad.");
            break;
        }
        System.out.println(portnamn + " kunde ej hittas. Försök igen.");
        }
        
    } // slut på hittaport
    
    private void anslut()
    {
        //Anslut till porten
        try{
            seriellport = (SerialPort)comport.open("V.E.N.T:Q",10000);
            System.out.println("Seriellport öppnad.");
            seriellport.setSerialPortParams(115200, SerialPort.DATABITS_8, SerialPort.STOPBITS_1, SerialPort.PARITY_NONE);
        //Thread.sleep(1000);
        } catch(Exception e) {
            System.err.println("Kunde inte ansluta.");
            flush();
            System.exit(1);
        }
        
        //Definiera in och utdata
        try{
        //Lägg till listerners som kan lyssna efter in-eller utdata
        
            seriellport.addEventListener(this);
            seriellport.notifyOnDataAvailable(true);
            seriellport.notifyOnOutputEmpty(true);
            //Kanske vill ha med framing error här senare 
            utdata = seriellport.getOutputStream();
            indata = seriellport.getInputStream();
            
        } catch (Exception e){
            System.err.println("Kunde inte definiera I/O.");
            //seriellport.close();
            flush();
            System.exit(1);
        }
    }
    
    public void flush()
    {
        indata = null;
        utdata = null;
        seriellport.close();
    }

    
    public boolean validHeader(int headerByte_) // Boolish
    {
        switch (headerByte_)
        {
            case 202: return true;  // Sida 1 (Norr)
            case 208: return true;  // Sida 2 (Öst)
            case 216: return true;  // Sida 3 (Cyd)
            case 224: return true;  // Sida 4 (Väst)
            case 232: return true;  // Vinkel 1 (Norr)
            case 240: return true;  // Vinkel 2 (Öst)
            case 248: return true;  // Vinkel 3 (Cyd)
            case 136: return true;  // Vinkel 4 (Väst)
            case 144: return true;  // Totalvinkel
            case 152: return true;  // Läcka
            case 160: return true;  // Nod
            default: return false;  // Inte en vettig header
        }
    }
    
    public boolean validData(int header_, int recievedData_)
    {
        switch(header_)
        {
            case 202: return ((recievedData_ > 70) && (recievedData_ <= 800 ));  // Avstånd kan bara vara mellan 70 och 800 mm         
            case 208: return ((recievedData_ > 70) && (recievedData_ <= 800 ));
            case 216: return ((recievedData_ > 70) && (recievedData_ <= 800 ));
            case 224: return ((recievedData_ > 70) && (recievedData_ <= 800 ));
            case 232: return ((recievedData_ > -410) && (recievedData_ < 410 )); // Vinkel kan bara vara mellan -410 och 410
            case 240: return ((recievedData_ > -410) && (recievedData_ < 410 ));
            case 248: return ((recievedData_ > -410) && (recievedData_ < 410 ));
            case 136: return ((recievedData_ > -410) && (recievedData_ < 410 ));
            case 144: return ((recievedData_ > -410) && (recievedData_ < 410 )); 
            case 152: return ((recievedData_ >= 0) && (recievedData_ <= 1024 ));  // Läcka
            case 160: return ((recievedData_ == 0) || (recievedData_ == 1));  // Nod
            default: return false;
        }
    }
    
    public int convertToSignedInt(int dataByte1_, int dataByte2_)
    {
        if (dataByte1_ > 127)
        {
            dataByte1_ = dataByte1_ - 128; // Nu kan byte 1 och 2 adderas
            return -(dataByte1_ * 256 + dataByte2_); // Adderar 1 för off-set
            
        }
        else
        {
            return dataByte1_ * 256 + dataByte2_; // Adderade
        }

    }
    
    
    //Hantera när data kommer in
    public void serialEvent(SerialPortEvent event)
    {
        if(event.getEventType() == SerialPortEvent.DATA_AVAILABLE)
        {
            //Gör det som ska göras vid inläsnign av data
            try{
            svar = indata.read(); //Kolla så att kommunikationsenheten fått tillbaka samma meddelande.

            //System.out.println(svar);
            
            
            dataBuffer[bufferCounter] = svar;
            System.out.print("Mottaget: ");
            System.out.println(svar);
            if(bufferCounter >= 2)
            {
                bufferCounter = 0;
                combinedData = convertToSignedInt(dataBuffer[1], dataBuffer[2]);
                if (validHeader(dataBuffer[0]) && validData(dataBuffer[0],combinedData))
                {
                    System.out.print("Resultat: ");
                    System.out.println(combinedData);
                }
                else
                {
                    System.out.print("Fel, buffer innehåller: ");
                    System.out.print(dataBuffer[0]);
                    System.out.print(" ");
                    System.out.print(dataBuffer[1]);
                    System.out.print(" ");
                    System.out.println(dataBuffer[2]);
                    
                    
                    dataBuffer[0] = dataBuffer[1];
                    dataBuffer[1] = dataBuffer[2];
                    bufferCounter = 2;
                }
            }
            else
            {
                bufferCounter ++;
            }
            
            
            } catch(Exception e){
                System.err.println("Kunde inte hämta data.");
                flush();
                System.exit(1);
            }
        }
    }
    
    public void skickainput()
    {
       try{
       //String intest = "D"; //Testkörning
           while(true)
       {
           String in = instruktion.next().toUpperCase(); 
           if((in.equals("Q"))||(in.equals("QUIT")))
           {
               flush();
               System.out.println("Seriellport stängd. Ha en fortsatt trevlig dag.");
               System.exit(1);
           }
           utdata.write(in.getBytes());
           
           //utdata.write(intest.getBytes()); //Testkörning
       }
        } catch(Exception e){
            System.err.println("Något gick fel med att skicka data.");
            //flush();
            //System.exit(1);
        }
    }
    
    public static void main(String[] args) {
        
        Interface test = new Interface();
        test.hittaport();
        test.anslut();
        test.skickainput();
    }
    
}