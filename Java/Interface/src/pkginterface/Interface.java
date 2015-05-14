/*
En liten tribute till rxtx-biblioteket: RXTX binary builds provided as a courtesy of Mfizz Inc. (http://mfizz.com/).
Please see http://mfizz.com/oss/rxtx-for-java for more information.
*/
package pkginterface;

import java.util.logging.Level;
import java.util.logging.Logger;
import javafx.scene.image.Image;
import jssc.SerialPort;
import static jssc.SerialPort.PURGE_RXCLEAR;
import jssc.SerialPortEvent;
import jssc.SerialPortEventListener;

public class Interface implements SerialPortEventListener{

    static SerialPort mainSerialPort;
    private static final int baud = 115200;
    private int response;
    private int[] dataBuffer = {0,0,0}; // Lagrar tre senaste hämtade bytes
    private int bufferCounter = 0; // Håller koll på var i buffern vi är
    private int combinedData = 0; // De två byten data, översatta till ett tal
    public String currentNode = "Okänt\nOkänt\nID: 0";
    public String currentNodeType = "";
    public String leak = "Nej";
    public String decision = "";
    public int acknowledge = 0;
    
    GUI mainGUI;
    
    public Interface(GUI creatingGUI)
    {
        mainGUI = creatingGUI;
    }
    
    /*void findPortGUI(String in)
    {
        
        CommPortIdentifier port = null; //Sätt nuvarande port till null
        Enumeration allPorts = CommPortIdentifier.getPortIdentifiers();
        while(allPorts.hasMoreElements())
        {
            port = (CommPortIdentifier)allPorts.nextElement();
            if(port.getName().equals(in))
            {                   //Gå igenom alla tillgängliga portar tills rätt port hittats!
                comPort = port;
                break;
            }
        }
        if(comPort != null)
        {
            System.out.println(in + " hittad.");
            portConnected = "Ansluten";
            mainGUI.setText("connection");
        }
        else
        {
            System.out.println(in + " kunde ej hittas. Försök igen.");
        }
    } // slut på hittaport*/
    
    void connect(String comPortName)
    {
        mainSerialPort = new SerialPort(comPortName); 
        //Anslut till porten
        try
        {
            mainSerialPort.openPort();
            mainSerialPort.setParams(115200, 8, 1, 0);//Set params
            int mask = SerialPort.MASK_RXCHAR;//Prepare mask
            mainSerialPort.setEventsMask(mask);//Set mask
            mainSerialPort.addEventListener(this);//Add SerialPortEventListener
            System.out.println("Seriellport öppnad.");
            mainGUI.portConnected = "Ansluten";
            mainGUI.setText("connection");
        }
        catch(Exception e)
        {
            System.err.println("Fel 1 vid anslutning:");
            System.err.println(e.toString());
        }
    }
    
    public void flush() // Änvänds för att koppla ifrån enheten
    {
        try
        {
            mainSerialPort.closePort();
            mainGUI.portConnected = "Ej ansluten";
            mainGUI.setText("connection");
            System.out.println("Seriell port bortkopplad");
        } 
        catch (Exception ex) 
        {
            Logger.getLogger(Interface.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    
    public boolean validHeader(int headerByte_) // Tar in en header och undersöker om den är en de 
    {                                           // headers som fördefinerats
        switch (headerByte_)
        {
            case 202: return true;  // Header för sida 1 (Norr)
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
            case 252: return true;  // Styrbeslut
                
            case 128: return true;  // Stegtid
            case 80: return true;   // Steglängd
            case 72: return true;   // Steghöjd
            case 64: return true;   // Utbredning
            case 56: return true;   // Höjd
            case 48: return true;   // Kvinkel
            case 40: return true;   // Ktranslation
            default: return false;  // Inte en vettig header
        }
    }
    
    public boolean validData(int header_, int recievedData_) // Tar in en header och data och undersöker om datan
    {                                                        // ligger innanför det intervall som är definerat för
        switch(header_)                                      // den headern
        {
            case 202: return ((recievedData_ > 70) && (recievedData_ <= 800 ));  // Avstånd kan bara vara mellan 70 och 800 mm         
            case 208: return ((recievedData_ > 70) && (recievedData_ <= 800 ));
            case 216: return ((recievedData_ > 70) && (recievedData_ <= 800 ));
            case 224: return ((recievedData_ > 70) && (recievedData_ <= 800 ));
            case 232: return ((recievedData_ > -410) && (recievedData_ <= 800 )); // Vinkel kan bara vara mellan -410 och 410
            case 240: return ((recievedData_ > -410) && (recievedData_ <= 800 ));
            case 248: return ((recievedData_ > -410) && (recievedData_ < 410 ));
            case 136: return ((recievedData_ > -410) && (recievedData_ < 410 ));
            case 144: return ((recievedData_ > -410) && (recievedData_ < 410 )); 
            case 152: return ((recievedData_ == 0) || (recievedData_ == 1));  // Läcka kan vara 1 eller 0
            case 160: return ((recievedData_ >= 0) && (recievedData_ <= 16400 ));  // Nod
            case 252: return ((recievedData_ >= 0) && (recievedData_ < 256));
                
            case 128: return ((recievedData_ >= 0)&&(recievedData_ <= 600));    
            case 80: return ((recievedData_ >= 0)&&(recievedData_ <= 500));    
            case 72: return ((recievedData_ >= 0)&&(recievedData_ <= 500));    
            case 64: return ((recievedData_ >= 0)&&(recievedData_ <= 500));    
            case 56: return ((recievedData_ >= 0)&&(recievedData_ <= 500));    
            case 48: return ((recievedData_ >= -10) &&(recievedData_ <= 10));    
            case 40: return ((recievedData_ >= -10) &&(recievedData_ <= 10));
            
            default: return false;
        }
    }
    
    /*public String getHeaderString(int header_)  // Tar in en header och returnerar vad den motsvarar i text
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
            case 252: return "Styrbeslut"; // Styrbeslut
            default: return "Fel, okänd data";  // Inte en vettig header
        }   
    }*/
    
    public void setNodeInfo(int north_, int east_, int south_, int west_, int direction_, int nodeID_) // Identifierar nod och riktning
    {                                                                                     //north_ - west_ är ett om roboten ser öppning, noll annars
        int sumDirections_ = north_ + east_ + south_ + west_; // Summa av öppningar roboten ser 
        String directionString_ = "";       // Vilken riktning roboten rör sig i
        switch(direction_)                  // 0 innebär norr, 1 innebär öst osv.
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
            case 4:
                directionString_ = "\nIngen riktning";
            default:
                directionString_ = "";
                break;
        }
        if(sumDirections_ == 3) // 3 öppningar innebär T-korsning
        {
            currentNode = "T-korsning" + directionString_ + "\nID: " + Integer.toString(nodeID_);
            if(north_ == 0)
            {
                currentNodeType = "T-korsning S";
            } else if(east_ == 0)
            {
                currentNodeType = "T-korsning V";
            } else if(south_ == 0)
            {
                currentNodeType = "T-korsning N";
            } else if(west_ == 0)
            {
                currentNodeType = "T-korsning Ö";
            }
        }
        if(sumDirections_ == 1) // 1 öppning innebär återvändsgränd
        {
            currentNode = "Återv.gränd" + directionString_ + "\nID: " + Integer.toString(nodeID_);
            if (north_ == 1)
            {
                currentNodeType = "Återv.gränd N";
            } else if (east_ == 1)
            {
                currentNodeType = "Återv.gränd Ö";
            } else if (south_ == 1)
            {
                currentNodeType = "Återv.gränd S";
            } else if (west_ == 1)
            {
                currentNodeType = "Återv.gränd V";
            }
        }
        if (sumDirections_ == 2) // Två öppningar betyder korridor eller sväng
        {                        // Undersöker om öppningarna är på motsatt sida av varandra,
            if((north_ * south_ == 1) || (east_ * west_ == 1)) //Isåfall korridor, annars sväng
            {
                currentNode = "Korridor" + directionString_ + "\nID: " + Integer.toString(nodeID_);
                if (north_ * south_ == 1)
                {
                    currentNodeType = "Korridor NS";
                } else
                {
                    currentNodeType = "Korridor ÖV";
                }
            }
            else
            {
                currentNode = "Sväng" + directionString_ + "\nID: " + Integer.toString(nodeID_);
                if (north_ * east_ == 1)
                {
                    currentNodeType = "Sväng NÖ";
                } else if (east_ * south_ == 1)
                {
                    currentNodeType = "Sväng SÖ";
                } else if (south_ * west_ == 1)
                {
                    currentNodeType = "Sväng SV";
                } else if (west_ * north_ == 1)
                {
                    currentNodeType = "Sväng NV";
                }
            }
        }
        if (sumDirections_ == 4)
        {
            currentNodeType = "Instängd";
        }
        if (sumDirections_ == 0)
        {
            currentNodeType = "Fyrv.korsn";
        }
    }
    
    public void setAllData(int header_, int recievedData_) // Tar in en header och data, behandlar data beroende på header
    {                                                      // och sparar den i lämplig variabel
        switch (header_)
        {
            case 202:                         // Om avstånd eller vinkel behöver datan inte behandlas
                mainGUI.allSides[0] = recievedData_;  // Sida 1 (Norr)
                mainGUI.setText("side1");
                break;
            case 208:
                mainGUI.allSides[1] = recievedData_;  // Sida 2 (Öst)
                mainGUI.setText("side2");
                break;
            case 216:
                mainGUI.allSides[2] = recievedData_;  // Sida 3 (Cyd)
                mainGUI.setText("side3");
                break;
            case 224:
                mainGUI.allSides[3] = recievedData_;  // Sida 4 (Väst)
                mainGUI.setText("side4");
                break;
            case 232:
                mainGUI.allAngles[0] = recievedData_;  // Vinkel 1 (Norr)
                mainGUI.setText("angle1");
                break;
            case 240:
                mainGUI.allAngles[1] = recievedData_;  // Vinkel 2 (Öst)
                mainGUI.setText("angle2");
                break;
            case 248:
                mainGUI.allAngles[2] = recievedData_;  // Vinkel 3 (Cyd)
                mainGUI.setText("angle3");
                break;
            case 136:
                mainGUI.allAngles[3] = recievedData_;  // Vinkel 4 (Väst)
                mainGUI.setText("angle4");
                break;
            case 144:
                mainGUI.allAngles[4] = recievedData_;  // Totalvinkel
                mainGUI.setText("angleTotal");
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
                mainGUI.setText("leak");
                break;
            case 160: 
                int northBit_ = (recievedData_ & 0b00001000)/8; // Bitarna 0,1,2,3 och  visar om det finns öppningar åt 
                int eastBit_ = (recievedData_ & 0b00000100)/4;  // väst, syd, öst respektive norr
                int southBit_ = (recievedData_ & 0b00000010)/2;
                int westBit_ = (recievedData_ & 0b00000001);
                int direction_ = (recievedData_ & 0b0000000100000000)/64 + (recievedData_ & 0b00100000)/16 + (recievedData_ & 0b00010000)/16; // Bitarna 4 och 5 är ett tal mellan 0 och 3 som
                                                                                                    // motsvarar åt vilket håll  roboten gick in i noden
                int IDbyte_ = (recievedData_ & 0b0011111100000000)/512;                             // Bitarna 8 till 13 är ett tal som motsvarar nodens Id
                setNodeInfo(northBit_,eastBit_,southBit_,westBit_,direction_,IDbyte_); // Beräknar vad det är för nod osv.
                mainGUI.setText("node");
                mainGUI.nodeImageView.setImage(mainGUI.nodeImageArray[recievedData_ & 0b00001111]);
                break;
            case 252:
                switch(recievedData_)
                {
                    case 0:
                        decision = "Norr";
                        mainGUI.setText("decision");
                        break;
                    case 1:
                        decision = "Öst";
                        mainGUI.setText("decision");
                        break;
                    case 2:
                        decision = "Syd";
                        mainGUI.setText("decision");
                        break;
                    case 3:
                        decision = "Väst";
                        mainGUI.setText("decision");
                        break;
                    case 4:
                        decision = "No Direction";
                        mainGUI.setText("decision");
                        break;
                    default:
                        break;      
                }
                break;
            case 128:
                mainGUI.timeOffset = recievedData_;
                mainGUI.setText("steptime");
                break;
            case 80:
                mainGUI.stepLengthOffset = recievedData_;
                mainGUI.setText("steplength");
                break;
            case 72:
                mainGUI.stepHeightOffset = recievedData_;
                mainGUI.setText("stepheight");
                break;
            case 64:
                mainGUI.xyWidthOffset = recievedData_;
                mainGUI.setText("xywidth");
                break;
            case 56:
                mainGUI.heightOffset = recievedData_;
                mainGUI.setText("height");
                break;
            case 48:
                mainGUI.kAngle = recievedData_;
                mainGUI.setText("kangle");
                break;
            case 40:
                mainGUI.kDistance = recievedData_;
                mainGUI.setText("kdistance");
                break;
            default: ;
        }  
        
    }
    
    public int convertToSignedInt(int dataByte1_, int dataByte2_) // Konverterar 2 separata byte till en signed int
    {
        if (dataByte1_ > 127)
        {
            return dataByte1_ * 256 + dataByte2_+ 0b11111111111111110000000000000000;
        }
        else
        {
            return dataByte1_ * 256 + dataByte2_; // Adderade
        }

    }
    
    
    //Hantera när data kommer in
    public void serialEvent(SerialPortEvent event)
    {
        if(event.isRXCHAR())
        {
            //Gör det som ska göras vid inläsnign av data
            try
            {
                //response = (mainSerialPort.readBytes(1)[0] & 0b11111111); //Kolla så att kommunikationsenheten fått tillbaka samma meddelande
                if (event.getEventValue() > 20)
                {
                    int recievedArray[] = mainSerialPort.readIntArray();
                    mainSerialPort.purgePort(PURGE_RXCLEAR);
                    for (int arrayIterator = 0; arrayIterator < recievedArray.length; arrayIterator ++)
                    {
                        response = recievedArray[arrayIterator];
                        if ((response == 184) && (bufferCounter == 0))
                        {
                            // 184 är en skräpheader som skickas ut vid kommunikationsenheten i samband med SPI-kommunkiation,
                        }   // den ska därför ignoreras om den är första delen av ett medelande
                        else
                        {
                            dataBuffer[bufferCounter] = response;
                            if(bufferCounter >= 2)
                            {
                                bufferCounter = 0;
                                combinedData = convertToSignedInt(dataBuffer[1], dataBuffer[2]);
                                if (validHeader(dataBuffer[0]) && validData(dataBuffer[0],combinedData))
                                {
                                    /*System.out.print(dataBuffer[0]);
                                    System.out.print(" ");
                                    System.out.print(dataBuffer[1]);
                                    System.out.print(" ");
                                    System.out.println(dataBuffer[2]);*/
                                    setAllData(dataBuffer[0],combinedData);
                                }
                                else
                                {
                                    dataBuffer[0] = dataBuffer[1];
                                    dataBuffer[1] = dataBuffer[2];
                                    bufferCounter = 2;
                                }
                            }
                            else
                            {
                                bufferCounter ++;
                            }
                        }
                    }
                }
            } 
            catch(Exception e)
            {
                System.err.println("Kunde inte hämta data.");
            }
        }
    }
    
    public void sendData(byte dataByte)
    {
        try
        {
            byte dataByteArray[] = {dataByte};
            mainSerialPort.writeBytes(dataByteArray);
        } 
        catch(Exception e)
        {
            System.err.println("Något gick fel med att skicka data.");
        }
        //waitForAcknowledge(dataByte);
    }
    
    public void waitForAcknowledge (byte dataByte)
    {
        try
        {
            Thread.sleep(1);                 //1000 milliseconds is one second.
        } 
        catch(InterruptedException ex)
        {
            Thread.currentThread().interrupt();
        }
        if (acknowledge != dataByte)
        {
            sendData(dataByte);
        }
    }
    
}