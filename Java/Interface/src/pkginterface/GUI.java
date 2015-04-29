
package pkginterface;


import javafx.geometry.Insets;
import javafx.application.Application;
import javafx.event.ActionEvent;
import javafx.event.EventHandler;
import javafx.geometry.Pos;
import javafx.scene.Group;
import javafx.scene.Scene;
import javafx.scene.chart.LineChart;
import javafx.scene.chart.NumberAxis;
import javafx.scene.chart.XYChart;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.TextArea;
import javafx.scene.control.TextField;
import javafx.scene.layout.GridPane;
import javafx.stage.Stage;
import javafx.scene.input.KeyEvent;
import javafx.scene.layout.Background;
import javafx.scene.layout.BackgroundFill;
import javafx.scene.layout.CornerRadii;
import javafx.scene.paint.Paint;
        
public class GUI extends Application
{
    private Interface mainInterface;
    private boolean autonomusMode = false;
    private int turnDirection = 0; // Vridning åt höger innebär positiv vridning, styrs av q och e
    private int upMovement = 0;    // Rörelse uppåt är positiv, styrs av w och s
    private int rightMovement = 0; // Rörelse åt höger är positiv, styrs av a och d
                                   // Om båda knapparna för motsvarande variabel är nedtryckt tar de ut varandra
                                   // och variabeln blir noll
    
    TextArea angle1Text = new TextArea(""); // Dessa texter skriver ut vinklar
    TextArea angle2Text = new TextArea("");
    TextArea angle3Text = new TextArea("");
    TextArea angle4Text = new TextArea("");
    TextArea angleTotalText = new TextArea("");
    TextArea side1Text = new TextArea(""); // Dessa texter skriver ut avstånd
    TextArea side2Text = new TextArea("");
    TextArea side3Text = new TextArea("");
    TextArea side4Text = new TextArea("");
    TextArea leakText = new TextArea("");  // leak, node, connected och autonomus visar robotens status
    TextArea nodeText = new TextArea("");
    TextArea connectedText = new TextArea("");
    TextArea autonomusText = new TextArea("Autonomt\nläge av");
    TextArea timeIncrementText = new TextArea("Stegtid:\n0");
    TextArea stepLengthText = new TextArea("Steglängd:\n0");
    TextArea stepHeightText = new TextArea("Steghöjd:\n0");
    TextArea xyWidthText = new TextArea("Utbredning:\n0");
    TextArea heightText = new TextArea("Höjd:\n0");
    TextField comXX = new TextField("COM15");
    Label upArrow = new Label("");   // Dessa texter använd för att visa riktning/vridning som nedtryckta
    Label downArrow = new Label(""); // knappar motsvarar
    Label leftArrow = new Label("");
    Label rightArrow = new Label("");
    Label turnSymbol = new Label("");
    boolean wPressed = false;  // Dessa värden används för att komma ihåg om en knapp är nedtryckt, sätts till sann
    boolean aPressed = false;  // när motsvarande knapp trycks ner och till falsk när knapp släpps
    boolean sPressed = false;
    boolean dPressed = false;
    boolean qPressed = false;
    boolean ePressed = false;
    boolean zPressed = false;
    boolean xPressed = false;
    boolean tPressed = false;
    boolean yPressed = false;
    boolean uPressed = false;
    boolean iPressed = false;
    boolean oPressed = false;
    
    int parameterChange = 0;
    
    int timeOffset = 0;
    int stepLengthOffset = 0;
    int stepHeightOffset = 0;
    int xyWidthOffset = 0;
    int heightOffset = 0;
    
    int timeUpperBound = 10;
    int stepLengthUpperBound = 10;
    int stepHeightUpperBound = 10;
    int xyWidthUpperBound = 10;
    int heightUpperBound = 10;
    
    int timeLowerBound = -10;
    int stepLengthLowerBound = -10;
    int stepHeightLowerBound = -10;
    int xyWidthLowerBound = -10;
    int heightLowerBound = -10;
    
    
    Background grayBackground = new Background(new BackgroundFill(Paint.valueOf("808080"),CornerRadii.EMPTY,Insets.EMPTY));
    Background redBackground = new Background(new BackgroundFill(Paint.valueOf("F00000"),CornerRadii.EMPTY,Insets.EMPTY));
    Background greenBackground = new Background(new BackgroundFill(Paint.valueOf("00F000"),CornerRadii.EMPTY,Insets.EMPTY));
    Background blackBackground = new Background(new BackgroundFill(Paint.valueOf("00000F"),CornerRadii.EMPTY,Insets.EMPTY));
    
    int side1Iterator = 0;  // Iteratorer för vilken datapunkt som tagits emot
    int side2Iterator = 0;
    int side3Iterator = 0;
    int side4Iterator = 0;
    int angle1Iterator = 0;
    int angle2Iterator = 0;
    int angle3Iterator = 0;
    int angle4Iterator = 0;
    int angleTotalIterator = 0;
    
    XYChart.Series side1Data = new XYChart.Series();
    XYChart.Series side2Data = new XYChart.Series();
    XYChart.Series side3Data = new XYChart.Series();
    XYChart.Series side4Data = new XYChart.Series();
    XYChart.Series angle1Data = new XYChart.Series();
    XYChart.Series angle2Data = new XYChart.Series();
    XYChart.Series angle3Data = new XYChart.Series();
    XYChart.Series angle4Data = new XYChart.Series();
    XYChart.Series angleTotalData = new XYChart.Series();
    

    void setMovement() // Omvandlar nedtryckta knappar till rörelseriktning och vridning. Skickar vidare
    {                  // eventuell rörelse via bluetooth och visar rörelse på GUI
        int movementByte_ = 0b00000000;  // I denna byte sätts olika bitar beroende på hur roboten ska röra sig, bitarna 0-3 motsvarar
                                         // en rörelsevektor och bitarna 4 och 5 motsvarar vridning
        switch(upMovement) // Undersöker om rörelse uppåt är positiv, negativ eller noll
        {
            case 0: 
                upArrow.setBackground(blackBackground);
                downArrow.setBackground(blackBackground);
                break;
            case 1:
                upArrow.setBackground(greenBackground); // Eventuell rörelse markeras med grönt
                downArrow.setBackground(blackBackground);
                movementByte_ = movementByte_ + 0b00000001;  // Bit 0 sätts till 1 om rörelsevektorn har rörelsekomponent uppåt
                break;                                       
            case -1:
                upArrow.setBackground(blackBackground);
                downArrow.setBackground(greenBackground);
                movementByte_ = movementByte_ + 0b00000010;  // Bit 1 sätts till 1 om rörelsevektorn har rörelsekomponent nedåt
                break;
            default:
                upArrow.setBackground(blackBackground);
                downArrow.setBackground(blackBackground);
                break;
                
        }
        switch(rightMovement) // Undersöker om rörelse åt höger är positiv, negativ eller noll
        {
            case 0:
                rightArrow.setBackground(blackBackground);
                leftArrow.setBackground(blackBackground);
                break;
            case 1:
                rightArrow.setBackground(greenBackground);
                leftArrow.setBackground(blackBackground);
                movementByte_ = movementByte_ + 0b00000100;  // Bit 2 sätts till 1 om rörelsevektorn har rörelsekomponent åt höger
                break;
            case -1:
                rightArrow.setBackground(blackBackground);
                leftArrow.setBackground(greenBackground);
                movementByte_ = movementByte_ + 0b00001000;  // Bit 3 sätts till 1 om rörelsevektorn har rörelsekomponent åt vänster
                break;
            default:
                rightArrow.setBackground(blackBackground);
                leftArrow.setBackground(blackBackground);
                break; 
                
        }
        
        switch(turnDirection) // Undersöker om vridning är positiv, negativ eller noll
        {
            case 0:
                turnSymbol.setText("");
                turnSymbol.setBackground(blackBackground);
                break;
            case 1:
                turnSymbol.setText("     H"); // Vridning markeras med bokstav för Höger eller Vänster
                turnSymbol.setBackground(greenBackground);
                movementByte_ = movementByte_ + 0b00010000;  // Bit 4 sätts till 1 om roboten ska vrida sig åt höger
                break;
            case -1:
                turnSymbol.setText("     V");
                turnSymbol.setBackground(greenBackground);
                movementByte_ = movementByte_ + 0b00100000;  // Bit 5 sätts till 1 om roboten ska vrida sig åt vänster
                break;
            default:
                turnSymbol.setText("");
                turnSymbol.setBackground(blackBackground);
                break;
        }
        mainInterface.sendData((byte)movementByte_);
        //System.out.println(movementByte_);
        
    }
    
    void sendChanges()
    {
        byte changeParameterByte = (byte)0b10000000;
        if (parameterChange == 1)
        {
            changeParameterByte += (byte)0b00000001;
        }
        
        if (parameterChange != 0)
        {
            if (tPressed)
            {
                if ((xyWidthOffset < xyWidthUpperBound)&&(parameterChange == 1))
                {
                    xyWidthOffset ++;
                }
                else if ((xyWidthOffset > xyWidthLowerBound)&&(parameterChange == -1))
                {
                    xyWidthOffset --;
                }
                xyWidthText.setText("Utbredning:\n" + Integer.toString(xyWidthOffset));
                mainInterface.sendData(changeParameterByte);
                System.out.println(changeParameterByte);
            }
            else if (yPressed)
            {
                if ((heightOffset < heightUpperBound)&&(parameterChange == 1))
                {
                    heightOffset ++;
                }
                else if ((heightOffset > heightLowerBound)&&(parameterChange == -1))
                {
                    heightOffset --;
                }
                heightText.setText("Höjd:\n" + Integer.toString(heightOffset));
                changeParameterByte += (byte)0b00000010;
                mainInterface.sendData(changeParameterByte);
                System.out.println(changeParameterByte);
            }
            else if (uPressed)
            {
                if ((stepLengthOffset < stepLengthUpperBound)&&(parameterChange == 1))
                {
                    stepLengthOffset ++;
                }
                else if ((stepLengthOffset > stepLengthLowerBound)&&(parameterChange == -1))
                {
                    stepLengthOffset --;
                }
                stepLengthText.setText("Steglängd:\n" + Integer.toString(stepLengthOffset));
                changeParameterByte += (byte)0b00000100;
                mainInterface.sendData(changeParameterByte);
                System.out.println(changeParameterByte);
            }
            
            else if (iPressed)
            {
                if ((stepHeightOffset < stepHeightUpperBound)&&(parameterChange == 1))
                {
                    stepHeightOffset ++;
                }
                else if ((stepHeightOffset > stepHeightLowerBound)&&(parameterChange == -1))
                {
                    stepHeightOffset --;
                }
                stepHeightText.setText("Steghöjd:\n" + Integer.toString(stepHeightOffset));
                changeParameterByte += (byte)0b00000110;
                mainInterface.sendData(changeParameterByte);
                System.out.println(changeParameterByte);
            }
            
            else if (oPressed)
            {
                if ((timeOffset < timeUpperBound)&&(parameterChange == 1))
                {
                    timeOffset ++;
                }
                else if ((timeOffset > timeLowerBound)&&(parameterChange == -1))
                {
                    timeOffset --;
                }
                timeIncrementText.setText("Stegtid:\n" + Integer.toString(timeOffset));
                changeParameterByte += (byte)0b00001000;
                mainInterface.sendData(changeParameterByte);
                System.out.println(changeParameterByte);
            }
        }
       
    }
        
    void setText(String updatedData_) // Hämtar variabler från mainInterface och sätter motsvarande text.
    {
        try
        {
            switch (updatedData_)
            {
                case "side1":
                {
                    
                    side1Data.getData().add(new XYChart.Data(side1Iterator,mainInterface.allSides[0]));
                    side1Text.setText("Sida 1:\n" + Integer.toString(mainInterface.allSides[0]));
                    side1Iterator ++;
                    break;
                }
                case "side2":
                {
                    side2Data.getData().add(new XYChart.Data(side2Iterator,mainInterface.allSides[1]));
                    side2Text.setText("Sida 2:\n" + Integer.toString(mainInterface.allSides[1]));
                    side2Iterator ++;
                    break;
                }
                case "side3":
                {
                    side3Data.getData().add(new XYChart.Data(side3Iterator,mainInterface.allSides[2]));
                    side3Text.setText("Sida 3:\n" + Integer.toString(mainInterface.allSides[2]));
                    side3Iterator ++;
                    break;
                }
                case "side4":
                {
                    side4Data.getData().add(new XYChart.Data(side4Iterator,mainInterface.allSides[3]));
                    side4Text.setText("Sida 4:\n" + Integer.toString(mainInterface.allSides[3]));
                    side4Iterator ++;
                    break;
                }
                case "angle1":
                {
                    angle1Data.getData().add(new XYChart.Data(angle1Iterator,mainInterface.allAngles[0]));
                    angle1Text.setText("Vinkel 1:\n" + Integer.toString(mainInterface.allAngles[0]));
                    angle1Iterator ++;
                    break;
                }
                case "angle2":
                {
                    angle2Data.getData().add(new XYChart.Data(angle2Iterator,mainInterface.allAngles[1]));
                    angle2Text.setText("Vinkel 2:\n" + Integer.toString(mainInterface.allAngles[1]));
                    angle2Iterator ++;
                    break;
                }
                case "angle3":
                {
                    angle3Data.getData().add(new XYChart.Data(angle3Iterator,mainInterface.allAngles[2]));
                    angle3Text.setText("Vinkel 3:\n" + Integer.toString(mainInterface.allAngles[2]));
                    angle3Iterator ++;
                    break;
                }
                case "angle4":
                {
                    angle4Data.getData().add(new XYChart.Data(angle4Iterator,mainInterface.allAngles[3]));
                    angle4Text.setText("Vinkel 4:\n" + Integer.toString(mainInterface.allAngles[3]));
                    angle4Iterator ++;
                    break;
                }
                case "angleTotal":
                {
                    angleTotalData.getData().add(new XYChart.Data(angleTotalIterator,mainInterface.allAngles[4]));
                    angleTotalText.setText("Vinkel total:\n" + Integer.toString(mainInterface.allAngles[4]));
                    angleTotalIterator ++;
                    break;
                }
                case "leak":
                {
                    leakText.setText("Läcka:\n" + mainInterface.leak);
                    break;
                }
                case "node":
                {
                    nodeText.setText("Nod:\n" +mainInterface.currentNode);
                    break;
                }
                case "connection":
                {
                    connectedText.setText("Seriell port: \n" + mainInterface.portConnected);
                    /*if(mainInterface.portConnected == "Ansluten")
                    {
                        connectedText.setTextFill(Paint.valueOf("00F000"));
                    }
                    else
                    {
                        connectedText.setTextFill(Paint.valueOf("F00000"));
                    }*/
                    break;
                }
                default:
                {
                    System.err.println("Faulty paramater to setText: " + updatedData_);
                }


            }
        }
        catch(Exception e)
        {
            System.err.println(e.toString());
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
        side1Text.setEditable(false);
        side2Text.setEditable(false);
        side3Text.setEditable(false);
        side4Text.setEditable(false);
        angle1Text.setEditable(false);
        angle2Text.setEditable(false);
        angle3Text.setEditable(false);
        angle4Text.setEditable(false);
        angleTotalText.setEditable(false);
        leakText.setEditable(false);
        nodeText.setEditable(false);
        
        timeIncrementText.setEditable(false);
        stepLengthText.setEditable(false);
        stepHeightText.setEditable(false);
        xyWidthText.setEditable(false);
        heightText.setEditable(false);
        
        side1Data.setName("Sida 1");
        side2Data.setName("Sida 2");
        side3Data.setName("Sida 3");
        side4Data.setName("Sida 4");
        angle1Data.setName("Vinkel 1");
        angle2Data.setName("Vinkel 2");
        angle3Data.setName("Vinkel 3");
        angle4Data.setName("Vinkel 4");
        angleTotalData.setName("Vinkel total");
        
        
        
        final NumberAxis sideAllXAxis = new NumberAxis();
        final NumberAxis sideAllYAxis = new NumberAxis();
        final LineChart<Number,Number> sideAllGraph = new LineChart<Number,Number>(sideAllXAxis,sideAllYAxis);
        sideAllGraph.setTitle("Alla sidor");
        sideAllGraph.getData().addAll(side1Data,side2Data,side3Data,side4Data);
        sideAllGraph.setCreateSymbols(false);
        
        
        final NumberAxis angleAllXAxis = new NumberAxis();
        final NumberAxis angleAllYAxis = new NumberAxis();
        final LineChart<Number,Number> angleAllGraph = new LineChart<Number,Number>(angleAllXAxis,angleAllYAxis);
        angleAllGraph.setTitle("Alla vinklar");
        angleAllGraph.getData().addAll(angle1Data,angle2Data,angle3Data,angle4Data,angleTotalData);
        angleAllGraph.setCreateSymbols(false);
        
       
        sideAllGraph.setMinWidth(800);
        angleAllGraph.setMinWidth(800);
        
        
        mainInterface = new Interface(this);
        
        stage.setTitle("V.E.N.T:Q Control Room");
        
        
        
        Group graphics = new Group();
        //Rectangle rektangel1 = new Rectangle(100,100,);
        
        Button connectButton = new Button(); // Skapar knapp som används för att ansluta till/koppla från roboten.
        connectButton.setText("    Anslut    "); // Om datorn ej ansluten till roboten används knappen för att ansluta,
        connectButton.setOnAction(new EventHandler<ActionEvent>() // annars använd den för att koppla från
        {
            @Override
            public void handle(ActionEvent event) // Funktion som sker när knappen trycks.
            {
                if (mainInterface.portConnected == "Ej ansluten") 
                {
                    mainInterface.findPortGUI(comXX.getText().toUpperCase()); // Försöker ansluta om ej ansluten
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
                    //System.out.println((byte)0b10000000);
                    autonomusMode = true;
                    autonomusButton.setText("Avaktivera");
                    autonomusText.setText("Autonomt\nläge på");
                    resetButtons(); // Nollställer allt och skickar det till robot så den ej fortsätter 
                }
                else  // Om i autonomt läge, växla till manuellt
                {
                    mainInterface.sendData((byte)0b11000000);
                   // System.out.println((byte)0b11000000);
                    autonomusMode = false;
                    autonomusButton.setText(" Aktivera ");
                    autonomusText.setText("Autonomt\nläge av");
                }
            }
        });
        
 
        
        
        GridPane root = new GridPane(); // Skapar en grid
        Scene mainScene = new Scene(root,1350,700); // Storlek är 1000x500 pixlar
        root.setAlignment(Pos.TOP_LEFT);
        root.setHgap(0); // Bestämmer horisontella och vertikala avstånd
        root.setVgap(0);
        root.setPadding(new Insets(10,25,25,10));
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
                        case "t":
                            if (!tPressed)
                            {
                                tPressed = true;
                            }
                            break;
                        case "y":
                            if (!yPressed)
                            {
                                yPressed = true;
                            }
                            break;
                        case "u":
                            if (!uPressed)
                            {
                                uPressed = true;
                            }
                            break;
                        case "i":
                            if (!iPressed)
                            {
                                iPressed = true;
                            }
                            break;
                        case "o":
                            if (!oPressed)
                            {
                                oPressed = true;
                            }
                            break;
                        case "z":
                            if (!zPressed)
                            {
                                parameterChange --;
                                zPressed = true;
                                sendChanges();
                            }
                            break;
                        case "x":
                            if (!xPressed)
                            {
                                parameterChange ++;
                                xPressed = true;
                                sendChanges();
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
                        case "t":
                            if (tPressed)
                            {
                                tPressed = false;
                          }
                            break;
                        case "y":
                            if (yPressed)
                            {
                                yPressed = false;
                          }
                            break;
                        case "u":
                            if (uPressed)
                            {
                                uPressed = false;
                          }
                            break;
                        case "i":
                            if (iPressed)
                            {
                                iPressed = false;
                          }
                            break;
                        case "o":
                            if (oPressed)
                            {
                                oPressed = false;
                          }
                            break;
                        case "z":
                            if (zPressed)
                            {
                                parameterChange ++;
                                zPressed = false;
                                sendChanges();
                          }
                            break;
                        case "x":
                            if (xPressed)
                            {
                                parameterChange --;
                                xPressed = false;
                                sendChanges();
                          }
                            break;
                        default:
                            break;
                    }
                }
            }
        });
        
        
        //root.setGridLinesVisible(true);
        root.setBackground(grayBackground);
        
        side1Text.setMinHeight(80);
        side1Text.setMinWidth(80);
        side2Text.setMinHeight(80);
        side2Text.setMinWidth(80);
        side3Text.setMinHeight(80);
        side3Text.setMinWidth(80);
        side4Text.setMinHeight(80);
        side4Text.setMinWidth(80);
        angle1Text.setMinHeight(80);
        angle1Text.setMinWidth(80);
        angle2Text.setMinHeight(80);
        angle2Text.setMinWidth(80);
        angle3Text.setMinHeight(80);
        angle3Text.setMinWidth(80);
        angle4Text.setMinHeight(80);
        angle4Text.setMinWidth(80);
        angleTotalText.setMinHeight(80);
        angleTotalText.setMinWidth(80);
        leakText.setMinHeight(80);
        leakText.setMinWidth(80);
        nodeText.setMinHeight(80);
        nodeText.setMinWidth(80);
        connectedText.setMinHeight(45);
        connectedText.setMinWidth(80);
        autonomusText.setMinHeight(45);
        autonomusText.setMinWidth(80);
        
        timeIncrementText.setMinHeight(45);
        timeIncrementText.setMinWidth(80);
        stepLengthText.setMinHeight(45);
        stepLengthText.setMinWidth(80);
        stepHeightText.setMinHeight(45);
        stepHeightText.setMinWidth(80);
        heightText.setMinHeight(45);
        heightText.setMinWidth(80);
        xyWidthText.setMinHeight(45);
        xyWidthText.setMinWidth(80);
        
        comXX.setMinHeight(30);
        comXX.setMinWidth(80);
        
        upArrow.setMinHeight(40);
        upArrow.setMinWidth(40);
        downArrow.setMinHeight(40);
        downArrow.setMinWidth(40);
        leftArrow.setMinHeight(40);
        leftArrow.setMinWidth(40);
        rightArrow.setMinHeight(40);
        rightArrow.setMinWidth(40);
        turnSymbol.setMinHeight(40);
        turnSymbol.setMinWidth(40);
        
        side1Text.setMaxWidth(80);
        side1Text.setMaxHeight(80);
        side2Text.setMaxWidth(80);
        side2Text.setMaxHeight(80);
        side3Text.setMaxWidth(80);
        side3Text.setMaxHeight(80);
        side4Text.setMaxWidth(80);
        side4Text.setMaxHeight(80);
        angle1Text.setMaxWidth(80);
        angle1Text.setMaxHeight(80);
        angle2Text.setMaxWidth(80);
        angle2Text.setMaxHeight(80);
        angle3Text.setMaxWidth(80);
        angle3Text.setMaxHeight(80);
        angle4Text.setMaxWidth(80);
        angle4Text.setMaxHeight(80);
        angleTotalText.setMaxWidth(80);
        angleTotalText.setMaxHeight(80);
        nodeText.setMaxWidth(80);
        nodeText.setMaxHeight(80);
        leakText.setMaxWidth(80);
        leakText.setMaxHeight(80);
        connectedText.setMaxWidth(80);
        connectedText.setMaxHeight(45);
        autonomusText.setMaxWidth(80);
        autonomusText.setMaxHeight(45);
        
        timeIncrementText.setMaxHeight(45);
        timeIncrementText.setMaxWidth(80);
        stepLengthText.setMaxHeight(45);
        stepLengthText.setMaxWidth(80);
        stepHeightText.setMaxHeight(45);
        stepHeightText.setMaxWidth(80);
        heightText.setMaxHeight(45);
        heightText.setMaxWidth(80);
        xyWidthText.setMaxHeight(45);
        xyWidthText.setMaxWidth(80);
        
        comXX.setMaxHeight(30);
        comXX.setMaxWidth(80);
        
        side1Text.setBackground(grayBackground);
        side2Text.setBackground(grayBackground);
        side3Text.setBackground(grayBackground);
        side4Text.setBackground(grayBackground);
        angle1Text.setBackground(grayBackground);
        angle2Text.setBackground(grayBackground);
        angle3Text.setBackground(grayBackground);
        angle4Text.setBackground(grayBackground);
        angleTotalText.setBackground(grayBackground);
        leakText.setBackground(grayBackground);
        nodeText.setBackground(grayBackground);
        
        
        

        
        root.add(side1Text,2,0,2,2); // Lägger till alla texter som hör till labels
        root.add(side2Text,6,2,2,2);
        root.add(side3Text,4,6,2,2);
        root.add(side4Text,0,4,2,2);
        root.add(angle1Text,4,0,2,2);
        root.add(angle2Text,6,4,2,2);
        root.add(angle3Text,2,6,2,2);
        root.add(angle4Text,0,2,2,2);
        root.add(angleTotalText,2,2,2,2);
        root.add(leakText,4,2,5,2);
        root.add(nodeText,2,4,2,2);
        
        
        root.add(connectedText,8,0);
        root.add(connectButton,8,1); // lägger till knappar och deras texter
        root.add(comXX,9,1,2,1);
        
        root.add(autonomusText,8,6);
        root.add(autonomusButton,8,7);
        
        //root.add(comXX,7,13); // Lägger till textfält för att välja comport
        
        root.add(leftArrow,9,3); // Lägger till symboler för att visa rörelseriktning
        root.add(rightArrow,11,3);
        root.add(upArrow,10,2);
        root.add(downArrow,10,4);
        root.add(turnSymbol,10,3);
        
        turnSymbol.setBackground(blackBackground); // Ändrar utseende på symbolerna
        
        
        root.add(sideAllGraph,12,0,3,8);
        root.add(angleAllGraph,12,8,3,8);
        
        root.add(timeIncrementText,0,8,2,1);
        root.add(stepLengthText,2,8,2,1);
        root.add(stepHeightText,4,8,2,1);
        root.add(xyWidthText,6,8,2,1);
        root.add(heightText,8,8,2,1);
        
        setText("side1");
        setText("side2");
        setText("side3");
        setText("side4");
        setText("angle1");
        setText("angle2");
        setText("angle3");
        setText("angle4");
        setText("angleTotal");
        setText("leak");
        setText("node");
        setText("connection");
        
        upArrow.setBackground(blackBackground);
        leftArrow.setBackground(blackBackground);
        rightArrow.setBackground(blackBackground);
        downArrow.setBackground(blackBackground);
    }
    
            
    public static void main(String[] args)
    {
        GUI mainGUI = new GUI();
        launch(args);
    }
}
