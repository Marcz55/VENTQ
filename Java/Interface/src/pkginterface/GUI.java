
package pkginterface;


import javafx.geometry.Insets;
import javafx.application.Application;
import javafx.event.ActionEvent;
import javafx.event.EventHandler;
import javafx.geometry.Pos;
import javafx.scene.Scene;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.layout.GridPane;
import javafx.scene.paint.Color;
import javafx.scene.text.Text;
import javafx.stage.Stage;
import javafx.scene.input.KeyEvent;
        
public class GUI extends Application
{
    private Interface mainInterface;
    private boolean autonomusMode = false;
    private int turnDirection = 0; // Vridning åt höger innebär positiv vridning, styrs av q och e
    private int upMovement = 0;    // Rörelse uppåt är positiv, styrs av w och s
    private int rightMovement = 0; // Rörelse åt höger är positiv, styrs av a och d
                                   // Om båda knapparna för motsvarande variabel är nedtryckt tar de ut varandra
                                   // och variabeln blir noll
    
    Text angle1Text = new Text(); // Dessa texter skriver ut vinklar
    Text angle2Text = new Text();
    Text angle3Text = new Text();
    Text angle4Text = new Text();
    Text angleTotalText = new Text();
    Text side1Text = new Text(); // Dessa texter skriver ut avstånd
    Text side2Text = new Text();
    Text side3Text = new Text();
    Text side4Text = new Text();
    Text leakText = new Text();  // leak, node, connected och autonomus visar robotens status
    Text nodeText = new Text();
    Text connectedText = new Text();
    Text autonomusText = new Text("Autonomt läge av");
    Text upArrow = new Text("^");   // Dessa texter använd för att visa riktning/vridning som nedtryckta
    Text downArrow = new Text("v"); // knappar motsvarar
    Text leftArrow = new Text("<");
    Text rightArrow = new Text(">");
    Text turnSymbol = new Text("");
    boolean wPressed = false;  // Dessa värden används för att komma ihåg om en knapp är nedtryckt, sätts till sann
    boolean aPressed = false;  // när motsvarande knapp trycks ner och till falsk när knapp släpps
    boolean sPressed = false;
    boolean dPressed = false;
    boolean qPressed = false;
    boolean ePressed = false;

    void setMovement() // Omvandlar nedtryckta knappar till rörelseriktning och vridning. Skickar vidare
    {                  // eventuell rörelse via bluetooth och visar rörelse på GUI
        int movementByte_ = 0b00000000;  // I denna byte sätts olika bitar beroende på hur roboten ska röra sig, bitarna 0-3 motsvarar
                                         // en rörelsevektor och bitarna 4 och 5 motsvarar vridning
        switch(upMovement) // Undersöker om rörelse uppåt är positiv, negativ eller noll
        {
            case 0: 
                upArrow.setFill(Color.BLACK);
                downArrow.setFill(Color.BLACK);
                break;
            case 1:
                upArrow.setFill(Color.LAWNGREEN); // Eventuell rörelse markeras med grönt
                downArrow.setFill(Color.BLACK);
                movementByte_ = movementByte_ + 0b00000001;  // Bit 0 sätts till 1 om rörelsevektorn har rörelsekomponent uppåt
                break;                                       
            case -1:
                upArrow.setFill(Color.BLACK);
                downArrow.setFill(Color.LAWNGREEN);
                movementByte_ = movementByte_ + 0b00000010;  // Bit 1 sätts till 1 om rörelsevektorn har rörelsekomponent nedåt
                break;
            default:
                upArrow.setFill(Color.BLACK);
                downArrow.setFill(Color.BLACK);
                break;
                
        }
        switch(rightMovement) // Undersöker om rörelse åt höger är positiv, negativ eller noll
        {
            case 0:
                rightArrow.setFill(Color.BLACK);
                leftArrow.setFill(Color.BLACK);
                break;
            case 1:
                rightArrow.setFill(Color.LAWNGREEN);
                leftArrow.setFill(Color.BLACK);
                movementByte_ = movementByte_ + 0b00000100;  // Bit 2 sätts till 1 om rörelsevektorn har rörelsekomponent åt höger
                break;
            case -1:
                rightArrow.setFill(Color.BLACK);
                leftArrow.setFill(Color.LAWNGREEN);
                movementByte_ = movementByte_ + 0b00001000;  // Bit 3 sätts till 1 om rörelsevektorn har rörelsekomponent åt vänster
                break;
            default:
                rightArrow.setFill(Color.BLACK);
                leftArrow.setFill(Color.BLACK);
                break; 
                
        }
        
        switch(turnDirection) // Undersöker om vridning är positiv, negativ eller noll
        {
            case 0:
                turnSymbol.setText("");
                break;
            case 1:
                turnSymbol.setText("H"); // Vridning markeras med bokstav för Höger eller Vänster
                movementByte_ = movementByte_ + 0b00010000;  // Bit 4 sätts till 1 om roboten ska vrida sig åt höger
                break;
            case -1:
                turnSymbol.setText("V");
                movementByte_ = movementByte_ + 0b00100000;  // Bit 5 sätts till 1 om roboten ska vrida sig åt vänster
                break;
            default:
                turnSymbol.setText("");
                break;
        }
        mainInterface.sendData((byte)movementByte_);
        
    }
        
    void setAllText() // Hämtar variabler från mainInterface och sätter motsvarande text.
    {
        side1Text.setText(Integer.toString(mainInterface.allSides[0]));
        side2Text.setText(Integer.toString(mainInterface.allSides[1]));
        side3Text.setText(Integer.toString(mainInterface.allSides[2]));
        side4Text.setText(Integer.toString(mainInterface.allSides[3]));
        angle1Text.setText(Integer.toString(mainInterface.allAngles[0]));
        angle2Text.setText(Integer.toString(mainInterface.allAngles[1]));
        angle3Text.setText(Integer.toString(mainInterface.allAngles[2]));
        angle4Text.setText(Integer.toString(mainInterface.allAngles[3]));
        angleTotalText.setText(Integer.toString(mainInterface.allAngles[4]));
        leakText.setText(mainInterface.leak);
        nodeText.setText(mainInterface.currentNode);
        connectedText.setText(mainInterface.portConnected);
        if(mainInterface.portConnected == "Ansluten")
        {
            connectedText.setFill(Color.LAWNGREEN);
        }
        else
        {
            connectedText.setFill(Color.CRIMSON);
        }
    }
    
    public void resetButtons() // Nollställer alla knappar
    {
        upMovement = 0;
        rightMovement = 0;
        turnDirection = 0;
        wPressed = false;
        aPressed = false;
        sPressed = false;
        dPressed = false;
        qPressed = false;
        ePressed = false;
        setMovement();
    }
    
    
    @Override
    public void start(Stage stage)
    {
        
        mainInterface = new Interface(this);
        
        stage.setTitle("V.E.N.T:Q Control Room");
        
        Button connectButton = new Button(); // Skapar knapp som används för att ansluta till/koppla från roboten.
        connectButton.setText("    Anslut    "); // Om datorn ej ansluten till roboten används knappen för att ansluta,
        connectButton.setOnAction(new EventHandler<ActionEvent>() // annars använd den för att koppla från
        {
            @Override
            public void handle(ActionEvent event) // Funktion som sker när knappen trycks.
            {
                if (mainInterface.portConnected == "Ej ansluten") 
                {
                    mainInterface.findPortGUI("COM12"); // Försöker ansluta om ej ansluten
                    if (mainInterface.comPort != null)  // Notera att COM** är olika på olika datorer
                    {
                        mainInterface.connect(); 
                    }
                    if(mainInterface.portConnected == "Ansluten") // Ändra knapp om anslutning lyckas
                    {
                        connectButton.setText("Koppla bort");
                    }
                }
                else
                {
                    resetButtons(); // Om ansluten, koppla från och ändra knapp, nollställ alla knapptryck
                    mainInterface.flush(); // och skicka till roboten så den inte fortsätter okontrollerat
                    connectButton.setText("    Anslut    ");
                }
            }
        });
        
        Button autonomusButton = new Button(); // Knapp för att byta mellan manuellt och autonomt värde
        autonomusButton.setText(" Aktivera ");
        autonomusButton.setOnAction(new EventHandler<ActionEvent>()
        {
            @Override
            public void handle(ActionEvent event)
            {
                if (!autonomusMode) // Om ej i autonomt läge, växla till autonomt läge genom att skicka rätt
                {                   // värde till robot och ändra på text och knapp
                    mainInterface.sendData((byte)0b10000000); 
                    System.out.println((byte)0b10000000);
                    autonomusMode = true;
                    autonomusButton.setText("Avaktivera");
                    autonomusText.setText("Autonomt läge på");
                    resetButtons(); // Nollställer allt och skickar det till robot så den ej fortsätter 
                }
                else  // Om i autonomt läge, växla till manuellt
                {
                    mainInterface.sendData((byte)0b11000000);
                    System.out.println((byte)0b11000000);
                    autonomusMode = false;
                    autonomusButton.setText(" Aktivera ");
                    autonomusText.setText("Autonomt läge av");
                }
            }
        });
        
        Label vinkel1 = new Label("Vinkel 1:"); // Labels för GUI:t
        Label vinkel2 = new Label("Vinkel 2:");
        Label vinkel3 = new Label("Vinkel 3:");
        Label vinkel4 = new Label("Vinkel 4:");
        Label vinkeltotal = new Label("Vinkel total:");
        Label sida1 = new Label("Sida 1:");
        Label sida2 = new Label("Sida 2:");
        Label sida3 = new Label("Sida 3:");
        Label sida4 = new Label("Sida 4:");
        Label läcka = new Label("Läcka:");
        Label nod = new Label("Nod:                  ");
        Label connected  = new Label("Seriell port: ");
        
        
        GridPane root = new GridPane(); // Skapar en grid
        Scene mainScene = new Scene(root,1000,500); // Storlek är 1000x500 pixlar
        root.setAlignment(Pos.CENTER);
        root.setHgap(50); // Bestämmer horisontella och vertikala avstånd
        root.setVgap(10);
        root.setPadding(new Insets(25,25,25,25));
        stage.setScene(mainScene);
        mainScene.getStylesheets().add(GUI.class.getResource("GUI.css").toExternalForm()); // Hämtar CSS där bakgrundsbild och vissa textdefinitioner finns
        stage.show(); 
        
        root.setOnKeyPressed(new EventHandler<KeyEvent>() // Eventhandler som hanterar när knapp trycks ner
        {
            public void handle(KeyEvent e)
            {
                if(e.getText().toLowerCase() == "r") // r används för "reset"
                {                                    // toLowerCase gör att capslock ej påverkar
                    resetButtons();
                }
                if (!autonomusMode) // Knapptryck för styrning registreras bara om roboten är i manuellt läge
                {
                    switch (e.getText().toLowerCase()) // Ändra up- och rightMovement samt turnDirection när
                    {                                  // knappar trycks ned
                                                       // w och s ökar respektive minskar upMovement, d och a ökar
                                                       // respektive minskar righMovement, e och q ökar respektive
                                                       // minskar turnDirection
                        case "w":                      
                            if (!wPressed)             // Knapptryck registreras ej om knappen redan nedtryckt
                            {                          
                                upMovement ++;
                                wPressed = true; 
                                setMovement();
                            }
                            break;
                        case "a":
                            if (!aPressed)
                            {
                                rightMovement --;
                                aPressed = true;
                                setMovement();
                            }
                            break;
                        case "s":
                            if (!sPressed)
                            {
                                upMovement --;
                                sPressed = true;
                                setMovement();
                            }
                            break;
                        case "d":
                            if (!dPressed)
                            {
                                rightMovement ++;
                                dPressed = true;
                                setMovement();
                            }
                            break;
                        case "q":
                            if (!qPressed)
                            {
                                turnDirection --;
                                qPressed = true;
                                setMovement();
                            }
                            break;
                        case "e":
                            if (!ePressed)
                            {
                                turnDirection ++;
                                ePressed = true;
                                setMovement();
                            }
                            break;
                        default:
                            break;
                    }
                }
            }
        });
        
        root.setOnKeyReleased(new EventHandler<KeyEvent>() // Eventhandler som hanterar när knapp släpps
        {                                                  // Knappuppsläpp fungerar omvänt som knapptryck,
            public void handle(KeyEvent e)                 // se funktion ovan
            {
                if (!autonomusMode)
                {
                    switch (e.getText().toLowerCase())
                    {
                        case "w":
                            if (wPressed)                 // Knappuppsläpp registreras bara om knapp redan nedtryckt
                            {
                                upMovement --;
                                wPressed = false;
                                setMovement();
                            }
                            break;
                        case "a":
                            if (aPressed)
                            {
                                rightMovement ++;
                                aPressed = false;
                                setMovement();
                            }
                            break;
                        case "s":
                            if (sPressed)
                            {
                                upMovement ++;
                                sPressed = false;
                                setMovement();
                            }
                            break;
                        case "d":
                            if (dPressed)
                            {
                                rightMovement --;
                                dPressed = false;
                                setMovement();
                            }
                            break;
                        case "q":
                            if (qPressed)
                            {
                                turnDirection ++;
                                qPressed = false;
                                setMovement();
                            }
                            break;
                        case "e":
                            if (ePressed)
                            {
                                turnDirection --;
                                ePressed = false;
                                setMovement();
                          }
                            break;
                        default:
                            break;
                    }
                }
            }
        });
        
        
        
        
        root.add(sida1,2,0); // Lägger till alla labels i den grid som skapats
        root.add(sida2,5,4);
        root.add(sida3,3,12);
        root.add(sida4,0,8);
        root.add(vinkel1,3,0);
        root.add(vinkel2,5,8);
        root.add(vinkel3,2,12);
        root.add(vinkel4,0,4);
        root.add(vinkeltotal,3,4);
        root.add(läcka,2,4);
        root.add(nod,2,8);
        
        root.add(side1Text,2,1); // Lägger till alla texter som hör till labels
        root.add(side2Text,5,5);
        root.add(side3Text,3,13);
        root.add(side4Text,0,9);
        root.add(angle1Text,3,1);
        root.add(angle2Text,5,9);
        root.add(angle3Text,2,13);
        root.add(angle4Text,0,5);
        root.add(angleTotalText,3,5);
        root.add(leakText,2,5);
        root.add(nodeText,2,9);
        
        root.add(connectButton,7,13); // lägger till knappar och deras texter
        root.add(connected,7,11);
        root.add(connectedText,7,12);
        
        root.add(autonomusText,7,0);
        root.add(autonomusButton,7,1);
        
        root.add(leftArrow,8,5); // Lägger till symboler för att visa rörelseriktning
        root.add(rightArrow,10,5);
        root.add(upArrow,9,3);
        root.add(downArrow,9,8);
        root.add(turnSymbol,9,5);
        
        turnSymbol.setFill(Color.LAWNGREEN); // Ändrar utseende på symbolerna
        upArrow.setId("boldText");
        leftArrow.setId("boldText");
        rightArrow.setId("boldText");
        turnSymbol.setId("boldText");
        
        setAllText();
    }
    
            
    public static void main(String[] args)
    {
        GUI mainGUI = new GUI();
        launch(args);
    }
}
