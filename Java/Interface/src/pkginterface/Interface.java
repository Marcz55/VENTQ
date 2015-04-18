/*
En liten tribute till rxtx-biblioteket: RXTX binary builds provided as a courtesy of Mfizz Inc. (http://mfizz.com/).
Please see http://mfizz.com/oss/rxtx-for-java for more information.
*/
package pkginterface;

import gnu.io.CommPortIdentifier;
import gnu.io.SerialPort;
import gnu.io.SerialPortEvent;
import gnu.io.SerialPortEventListener;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Enumeration;
import java.util.Scanner;
import java.util.logging.Level;
import java.util.logging.Logger;
import javafx.scene.paint.Color;

public class Interface implements SerialPortEventListener{

    private SerialPort seriellport;
    public CommPortIdentifier comport = null;
    private static final int baud = 115200;
    private String portnamn = "Ingenting";
    private int svar;
    private InputStream indata;
    private OutputStream utdata;
    Scanner instruktion = new Scanner(System.in);
    private int[] dataBuffer = {0,0,0}; // Lagrar tre senaste hämtade bytes
    private int bufferCounter = 0; // Håller koll på var i buffern vi är
    private int combinedData = 0; // De två byten data, översatta till ett tal
    public int[] allAngles = {0,0,0,0,0};
    public int[] allSides = {0,0,0,0};
    public String currentNode = "Korridor";
    public String leak = "Nej";
    public String portConnected = "Ej ansluten";
    
    GUI mainGUI;
    
    public Interface(GUI creatingGUI)
    {
        mainGUI = creatingGUI;
    }
   
    void hittaport()
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
    
    void hittaportGUI(String in)
    {
        portnamn = in;
        
        CommPortIdentifier port = null; //Sätt nuvarande port till null
        Enumeration allaportar = CommPortIdentifier.getPortIdentifiers();
        while(allaportar.hasMoreElements())
        {
            port = (CommPortIdentifier)allaportar.nextElement();
            if(port.getName().equals(portnamn))
            {                   //Gå igenom alla tillgängliga portar tills rätt port hittats!
                comport = port;
                break;
            }
        }
        if(comport != null)
        {
            System.out.println(portnamn + " hittad.");
            portConnected = "Ansluten";
            mainGUI.setAllText();
        }
        else
        {
            System.out.println(portnamn + " kunde ej hittas. Försök igen.");
        }
        
        
    } // slut på hittaport
    
    void anslut()
    {
        //Anslut till porten
        try
        {
            System.out.println(comport);
            seriellport = (SerialPort)comport.open("V.E.N.T:Q",2000);
            System.out.println("Seriellport öppnad.");
            seriellport.setSerialPortParams(115200, SerialPort.DATABITS_8, SerialPort.STOPBITS_1, SerialPort.PARITY_NONE);
        //Thread.sleep(1000);
        }
        catch(Exception e)
        {
            System.err.println("Fel 1 vid anslutning:");
            System.err.println(e.toString());
            //flush();
            //System.exit(1);
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
            System.err.println("Fel 2 vid anslutning:");
            System.err.println(e.toString());
            //seriellport.close();
            //flush();
            //System.exit(1);
        }
    }
    
    /*void anslutGUI()
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
    }*/
    
    public void flush()
    {
        try {
            indata.close();
            utdata.close();
            indata = null;
            utdata = null;
            seriellport.close();
            portConnected = "Ej ansluten";
            mainGUI.setAllText();
            System.out.println("Seriell port bortkopplad");
            comport = null;
        } catch (IOException ex) {
            Logger.getLogger(Interface.class.getName()).log(Level.SEVERE, null, ex);
        }
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
            case 152: return ((recievedData_ == 0) || (recievedData_ == 1));  // Läcka
            case 160: return ((recievedData_ >= 0) && (recievedData_ <= 16400 ));  // Nod
            default: return false;
        }
    }
    
    public String getHeaderString(int header_)
    {
        switch (header_)
        {
            case 202: return "Sida 1: ";  // Sida 1 (Norr)
            case 208: return "Sida 2: ";  // Sida 2 (Öst)
            case 216: return "Sida 3: ";  // Sida 3 (Cyd)
            case 224: return "Sida 4: ";  // Sida 4 (Väst)
            case 232: return "Vinkel 1: ";  // Vinkel 1 (Norr)
            case 240: return "Vinkel 2: ";  // Vinkel 2 (Öst)
            case 248: return "Vinkel 3: ";  // Vinkel 3 (Cyd)
            case 136: return "Vinkel 4: ";  // Vinkel 4 (Väst)
            case 144: return "Vinkel total: ";  // Totalvinkel
            case 152: return "Läcka: ";  // Läcka
            case 160: return "Nod: ";  // Nod
            default: return "Fel, okänd data";  // Inte en vettig header
        }   
    }
    
    public void setNodeInfo(int north_, int east_, int south_, int west_, int direction_, int nodeID_) // Identifierar nod och riktning
    {                                                                                     //north - west är ett om roboten ser öppning, noll annars
        int sumDirections_ = north_ + east_ + south_ + west_; // Summa av öppningar roboten ser 
        String directionString_ = "";       // Vilken riktning roboten rör sig i
        switch(direction_)             // 0 innebär norr, 1 innebär öst osv.
        {
            case 0:
                directionString_ = "\nNorr";
                break;
            case 1:
                directionString_ = "\nÖst";
                break;
            case 2:
                directionString_ = "\nSyd";
                break;
            case 3:
                directionString_ = "\nVäst";
                break;             
            default:
                directionString_ = "";
                break;
        }
        if(sumDirections_ == 3) // 3 öppningar innebär T-korsning
        {
            currentNode = "T-korsning" + directionString_ + "\nID: " + Integer.toString(nodeID_); 
        }
        if(sumDirections_ == 1) // 1 öppning innebär återvändsgränd
        {
            currentNode = "Återvändsgränd" + directionString_ + "\nID: " + Integer.toString(nodeID_);
        }
        if (sumDirections_ == 2) // Två öppningar betyder korridor eller sväng
        {                        // Undersöker om öppningarna är på motsatt sida av varandra,
            if((north_ * south_ == 1) || (east_ * west_ == 1)) //Isåfall korridor, annars sväng
            {
                currentNode = "Korridor" + directionString_ + "\nID: " + Integer.toString(nodeID_); 
            }
            else
            {
                currentNode = "Sväng" + directionString_ + "\nID: " + Integer.toString(nodeID_);
            }
        }
    }
    
    public void setAllData(int header_, int recievedData_)
    {
        switch (header_)
        {
            case 202:
                allSides[0] = recievedData_;  // Sida 1 (Norr)
                break;
            case 208:
                allSides[1] = recievedData_;  // Sida 2 (Öst)
                break;
            case 216:
                allSides[2] = recievedData_;  // Sida 3 (Cyd)
                break;
            case 224:
                allSides[3] = recievedData_;  // Sida 4 (Väst)
                break;
            case 232:
                allAngles[0] = recievedData_;  // Vinkel 1 (Norr)
                break;
            case 240:
                allAngles[1] = recievedData_;  // Vinkel 2 (Öst)
                break;
            case 248:
                allAngles[2] = recievedData_;  // Vinkel 3 (Cyd)
                break;
            case 136:
                allAngles[3] = recievedData_;  // Vinkel 4 (Väst)
                break;
            case 144:
                allAngles[4] = recievedData_;  // Totalvinkel
                break;
            case 152: // Recieved data motsvarar om läcka är sant eller falskt
                if (recievedData_ == 1)
                {
                    leak = "Ja";
                }
                else
                {
                    leak = "Nej";
                }
                break;
            case 160:
                int northBit_ = (recievedData_ & 0b00001000)/8;
                int eastBit_ = (recievedData_ & 0b00000100)/4;
                int southBit_ = (recievedData_ & 0b00000010)/2;
                int westBit_ = (recievedData_ & 0b00000001);
                int direction_ = (recievedData_ & 0b00100000)/16 + (recievedData_ & 0b00010000)/16;
                int IDbyte_ = (recievedData_ & 0b0011111100000000)/256;
                setNodeInfo(northBit_,eastBit_,southBit_,westBit_,direction_,IDbyte_);  // Läcka
                break;
            default: ;
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
            if(bufferCounter >= 2)
            {
                bufferCounter = 0;
                combinedData = convertToSignedInt(dataBuffer[1], dataBuffer[2]);
                if (validHeader(dataBuffer[0]) && validData(dataBuffer[0],combinedData))
                {
                    setAllData(dataBuffer[0],combinedData);
                    mainGUI.setAllText();
                }
                else
                {
                    /*
                    System.out.print("Fel, buffer innehåller: ");
                    System.out.print(dataBuffer[0]);
                    System.out.print(" ");
                    System.out.print(dataBuffer[1]);
                    System.out.print(" ");
                    System.out.println(dataBuffer[2]);
                    */
                    
                    
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
            }
        }
    }
    
    public void skickainput(String dataToSend)
    {
       try{
       //String intest = "D"; //Testkörning
           //dataToSend = instruktion.next().toUpperCase(); 
           if((dataToSend.equals("Q"))||(dataToSend.equals("QUIT")))
           {
               flush();
               System.out.println("Seriellport stängd. Ha en fortsatt trevlig dag.");
               System.exit(1);
           }
           utdata.write(dataToSend.getBytes());
           
           //utdata.write(intest.getBytes()); //Testkörning
       
        } catch(Exception e){
            System.err.println("Något gick fel med att skicka data.");
            //flush();
            //System.exit(1);
        }
    }
    
}