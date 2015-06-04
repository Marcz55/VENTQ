
package pkginterface;


import javafx.geometry.Insets;
import javafx.application.Application;
import javafx.application.Platform;
import javafx.concurrent.Task;
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
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.layout.GridPane;
import javafx.stage.Stage;
import javafx.scene.input.KeyEvent;
import javafx.scene.input.MouseEvent;
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
    
    int timeOffset = 0;//200;
    int stepLengthOffset = 0;//160;
    int stepHeightOffset = 0;//40;
    int xyWidthOffset = 0;//80;
    int heightOffset = 0;//120;
    int kDistance = 0;//30;
    int kAngle = 0;//80;
    
    public Button connectButton = new Button(); // Skapar knapp som används för att ansluta till/koppla från roboten.
    public Button findLeakButton = new Button(); //Skapar knapp som används för att välja läcka att gå till.
    
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
    TextArea timeIncrementText = new TextArea("Upp/o\nStegtid:\n" + Integer.toString(timeOffset));
    TextArea stepLengthText = new TextArea("X/u\nSteglängd:\n" + Integer.toString(stepLengthOffset));
    TextArea stepHeightText = new TextArea("Y/i\nSteghöjd:\n" + Integer.toString(stepHeightOffset));
    TextArea xyWidthText = new TextArea("A/t\nUtbredning:\n" + Integer.toString(xyWidthOffset));
    TextArea heightText = new TextArea("B/y\nHöjd:\n" + Integer.toString(heightOffset));
    TextArea kDistanceText = new TextArea("Vänster/g\nkDistance:\n" + Double.toString((double)kDistance/100));
    TextArea kAngleText = new TextArea("Höger/h\nkAngle:\n" + Double.toString((double)kAngle/100));
    TextArea decisionList = new TextArea("Styrbeslut:");
    TextArea nodeList = new TextArea("Noder:");
    TextField findLeak = new TextField("1");
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
    boolean gPressed = false;
    boolean hPressed = false;
    
    int parameterChange = 0;
    
    String setConnect = "Standby";
    boolean graphStarted = false;
    
    int graphIterator = 0;
    double graphUpperBound = 10;
    double sideGraphUpperBound = graphUpperBound;
    double angleGraphUpperBound = graphUpperBound;
    double graphUpdateTime = 0.5;
    final NumberAxis sideAllXAxis = new NumberAxis();
    final NumberAxis sideAllYAxis = new NumberAxis();
    final NumberAxis angleAllXAxis = new NumberAxis();
    final NumberAxis angleAllYAxis = new NumberAxis();
    final LineChart<Number,Number> angleAllGraph = new LineChart<Number,Number>(angleAllXAxis,angleAllYAxis);
    final LineChart<Number,Number> sideAllGraph = new LineChart<Number,Number>(sideAllXAxis,sideAllYAxis);
    
    // Färger som kan användas
    Background grayBackground = new Background(new BackgroundFill(Paint.valueOf("808080"),CornerRadii.EMPTY,Insets.EMPTY));
    Background redBackground = new Background(new BackgroundFill(Paint.valueOf("F00000"),CornerRadii.EMPTY,Insets.EMPTY));
    Background greenBackground = new Background(new BackgroundFill(Paint.valueOf("00F000"),CornerRadii.EMPTY,Insets.EMPTY));
    Background blackBackground = new Background(new BackgroundFill(Paint.valueOf("00000F"),CornerRadii.EMPTY,Insets.EMPTY));
    
    boolean leakAdded = false;
    
    int allSides[] = {0,0,0,0};
    int allAngles[] = {0,0,0,0,0};
    
    XYChart.Series side1Data = new XYChart.Series();
    XYChart.Series side2Data = new XYChart.Series();
    XYChart.Series side3Data = new XYChart.Series();
    XYChart.Series side4Data = new XYChart.Series();
    XYChart.Series angle1Data = new XYChart.Series();
    XYChart.Series angle2Data = new XYChart.Series();
    XYChart.Series angle3Data = new XYChart.Series();
    XYChart.Series angle4Data = new XYChart.Series();
    XYChart.Series angleTotalData = new XYChart.Series();
    
    public String portConnected = "Ej ansluten";
    
    // Bilder av noder
    public Image[] nodeImageArray = 
    {new Image(getClass().getResourceAsStream("0.png")),
    new Image(getClass().getResourceAsStream("1.png")),
    new Image(getClass().getResourceAsStream("2.png")),
    new Image(getClass().getResourceAsStream("3.png")),
    new Image(getClass().getResourceAsStream("4.png")),
    new Image(getClass().getResourceAsStream("5.png")),
    new Image(getClass().getResourceAsStream("6.png")),
    new Image(getClass().getResourceAsStream("7.png")),
    new Image(getClass().getResourceAsStream("8.png")),
    new Image(getClass().getResourceAsStream("9.png")),
    new Image(getClass().getResourceAsStream("10.png")),
    new Image(getClass().getResourceAsStream("11.png")),
    new Image(getClass().getResourceAsStream("12.png")),
    new Image(getClass().getResourceAsStream("13.png")),
    new Image(getClass().getResourceAsStream("14.png")),
    new Image(getClass().getResourceAsStream("15.png"))};
    public ImageView nodeImageView = new ImageView();

    void updateNodeText()
    {
        nodeText.setText("Nod:\n" +mainInterface.currentNode);
        nodeList.appendText(mainInterface.currentNodeType + "\n");
    }
    
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
        
    }
    
    void sendChanges() // Skapar en byte som säger vilken parameter som ska ändras och hur
    {
        byte changeParameterByte = (byte)0b10000000;
        if (parameterChange == 1)
        {
            changeParameterByte += (byte)0b00000001; // Etta eller nolla på denna bit betyder att parametern ska ökas respektive sänkas.
        }
        
        if (parameterChange != 0) // Här sätts tre bitar utifrån vilken parameter som ska ändras-
        {
            if (tPressed)
            {
                mainInterface.sendData(changeParameterByte);
            }
            else if (yPressed)
            {
                changeParameterByte += (byte)0b00000010;
                mainInterface.sendData(changeParameterByte);
            }
            else if (uPressed)
            {
                changeParameterByte += (byte)0b00000100;
                mainInterface.sendData(changeParameterByte);
            }
            
            else if (iPressed)
            {
                changeParameterByte += (byte)0b00000110;
                mainInterface.sendData(changeParameterByte);
            }
            
            else if (oPressed)
            {
                changeParameterByte += (byte)0b00001000;
                mainInterface.sendData(changeParameterByte);
            }
            else if (gPressed)
            {
                changeParameterByte += (byte)0b00001010;
                mainInterface.sendData(changeParameterByte);
            }
            else if (hPressed)
            {
                changeParameterByte += (byte)0b00001100;
                mainInterface.sendData(changeParameterByte);
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
                    side1Text.setText("Sida 1:\n" + Integer.toString(allSides[0]));
                    break;
                }
                case "side2":
                {
                    side2Text.setText("Sida 2:\n" + Integer.toString(allSides[1]));
                    break;
                }
                case "side3":
                {
                    side3Text.setText("Sida 3:\n" + Integer.toString(allSides[2]));
                    break;
                }
                case "side4":
                {
                    side4Text.setText("Sida 4:\n" + Integer.toString(allSides[3]));
                    break;
                }
                case "angle1":
                {
                    angle1Text.setText("Vinkel 1:\n" + Integer.toString(allAngles[0]));
                    break;
                }
                case "angle2":
                {
                    angle2Text.setText("Vinkel 2:\n" + Integer.toString(allAngles[1]));
                    break;
                }
                case "angle3":
                {
                    angle3Text.setText("Vinkel 3:\n" + Integer.toString(allAngles[2]));
                    break;
                }
                case "angle4":
                {
                    angle4Text.setText("Vinkel 4:\n" + Integer.toString(allAngles[3]));
                    break;
                }
                case "angleTotal":
                {
                    angleTotalText.setText("Vinkel total:\n" + Integer.toString(allAngles[4]));
                    break;
                }
                case "leak":
                {
                    leakText.setText("Läcka:\n" + mainInterface.leak);
                    if (mainInterface.leak == "Nej")
                    {
                        leakAdded = false;
                    }
                    else if (mainInterface.leak == "Ja")
                    {
                        if (!leakAdded)
                        {
                            nodeList.appendText("\nLäcka hittad!\n\n");
                            decisionList.appendText("\nLäcka hittad!\n\n");
                            leakAdded = true;
                        }
                    }
                    break;
                }
                case "node":
                {
                    updateNodeText();
                    break;
                }
                case "connection":
                {
                    connectedText.setText("Seriell port: \n" + portConnected);
                    break;
                }
                case "decision":
                {
                    decisionList.appendText(mainInterface.decision + "\n");
                    break;
                }
                
                case "steptime":
                {
                    timeIncrementText.setText("Upp/o\nStegtid:\n" + Integer.toString(timeOffset));
                    break;
                }
                case "steplength":
                {
                    stepLengthText.setText("X/u\nSteglängd:\n" + Integer.toString(stepLengthOffset));
                    break;
                }
                case "stepheight":
                {
                    stepHeightText.setText("Y/i\nSteghöjd:\n" + Integer.toString(stepHeightOffset));
                    break;
                }
                case "xywidth":
                {
                    xyWidthText.setText("A/t\nUtbredning:\n" + Integer.toString(xyWidthOffset));
                    break;
                }
                case "height":
                {
                    heightText.setText("B/y\nHöjd:\n" + Integer.toString(heightOffset));
                    break;
                }
                case "kangle":
                {
                    kAngleText.setText("Höger/h\nkAngle:\n" + Double.toString((double)kAngle/100));
                    break;
                }
                case "kdistance":
                {
                    kDistanceText.setText("Vänster/g\nkDistance:\n" + Double.toString((double)kDistance/100));
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
    
    public void getGraphData(double dataIterations_) // Uppdaterar grafernas data
    {
        try
        {
            side1Data.getData().add(new XYChart.Data(graphUpdateTime*dataIterations_,allSides[0]));
            side2Data.getData().add(new XYChart.Data(graphUpdateTime*dataIterations_,allSides[1]));
            side3Data.getData().add(new XYChart.Data(graphUpdateTime*dataIterations_,allSides[2]));
            side4Data.getData().add(new XYChart.Data(graphUpdateTime*dataIterations_,allSides[3]));
            angle1Data.getData().add(new XYChart.Data(graphUpdateTime*dataIterations_,allAngles[0]));
            angle2Data.getData().add(new XYChart.Data(graphUpdateTime*dataIterations_,allAngles[1]));
            angle3Data.getData().add(new XYChart.Data(graphUpdateTime*dataIterations_,allAngles[2]));
            angle4Data.getData().add(new XYChart.Data(graphUpdateTime*dataIterations_,allAngles[3]));
            angleTotalData.getData().add(new XYChart.Data(graphUpdateTime*dataIterations_,allAngles[4]));
            if (dataIterations_ > graphUpperBound/graphUpdateTime)
            {
                sideGraphUpperBound += graphUpdateTime;
                angleGraphUpperBound += graphUpdateTime;
                sideAllXAxis.setUpperBound(sideGraphUpperBound);
                sideAllXAxis.setLowerBound(sideGraphUpperBound - graphUpperBound);
                angleAllXAxis.setUpperBound(angleGraphUpperBound);
                angleAllXAxis.setLowerBound(angleGraphUpperBound - graphUpperBound);
            }
        }
        catch(Exception e)
        {
            System.err.println(e.toString());
        }
    }
    
    public void resetGraphs() // Rensar alla grafer. Intabil, använd med försiktighet.
    {
      
        side1Data.getData().clear();
        side1Data.getData().clear();
        side2Data.getData().clear();
        side3Data.getData().clear();
        side4Data.getData().clear();
        angle1Data.getData().clear();
        angle2Data.getData().clear();
        angle3Data.getData().clear();
        angle4Data.getData().clear();
        angleTotalData.getData().clear(); 
                 
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
        
        Task<Integer> graphTask = new Task<Integer>() // Denna tråd uppdaterar grafer med jämna mellanrum.
        {
            @Override protected Integer call() throws Exception
            {
                while (true)
                {
                    if (isCancelled())
                    {
                        break;
                    }
                    Platform.runLater(new Runnable()
                    {
                        @Override
                        public void run()
                        {
                            getGraphData(graphIterator);
                        }
                    });
                    graphIterator ++;
                    double sleepTime = 1000*graphUpdateTime;
                    Thread.sleep((long)sleepTime);
                }
                return 0;
             }
         };
        
        Thread graphThread = new Thread(graphTask);
        graphThread.setDaemon(true);
        
        
        Task<Integer> connectTask = new Task<Integer>() // Denna tråd undersöker med jämna mellanrum om den ska ansluta till 
        {                                               // eller koppla ifrån Bluetoothenheten.
            @Override protected Integer call() throws Exception
            {
                while (true)
                {
                    if (isCancelled())
                    {
                        break;
                    }
                    if (setConnect == "Connect")
                    {
                        mainInterface.connect(comXX.getText());
                        if ((portConnected == "Ansluten") && !graphStarted)
                        {
                            graphThread.start();
                            graphStarted = true;
                        }
                        setConnect = "standby";
                    }
                    else if (setConnect == "Disconnect")
                    {
                        mainInterface.flush();
                        setConnect = "Standby";
                    }
                    Thread.sleep(1000);
                }
                return 0;
            }
        };
        
        
        
        Thread connectThread = new Thread(connectTask);
        connectThread.setDaemon(true);
        connectThread.start();
        
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
        decisionList.setEditable(false);
        nodeList.setEditable(false);
        
        timeIncrementText.setEditable(false);
        stepLengthText.setEditable(false);
        stepHeightText.setEditable(false);
        xyWidthText.setEditable(false);
        heightText.setEditable(false);
        kDistanceText.setEditable(false);
        kAngleText.setEditable(false);
        
        side1Data.setName("Norr");
        side2Data.setName("Öst");
        side3Data.setName("Syd");
        side4Data.setName("Väst");
        angle1Data.setName("Vinkel norr");
        angle2Data.setName("Vinkel öst");
        angle3Data.setName("Vinkel syd");
        angle4Data.setName("Vinkel väst");
        angleTotalData.setName("Vinkel total");
        
        
        
        
        sideAllGraph.setTitle("Alla sidor");
        sideAllGraph.getData().addAll(side1Data,side2Data,side3Data,side4Data);
        sideAllGraph.setCreateSymbols(false);
        sideAllXAxis.setAutoRanging(false);
        sideAllYAxis.setAutoRanging(false);
        sideAllXAxis.setUpperBound(sideGraphUpperBound);
        sideAllYAxis.setUpperBound(800);
        sideAllYAxis.setLowerBound(0);
        
        
        angleAllGraph.setTitle("Alla vinklar");
        angleAllGraph.getData().addAll(angle1Data,angle2Data,angle3Data,angle4Data,angleTotalData);
        angleAllGraph.setCreateSymbols(false);
        angleAllXAxis.setAutoRanging(false);
        angleAllYAxis.setAutoRanging(false);
        angleAllXAxis.setUpperBound(angleGraphUpperBound);
        angleAllYAxis.setUpperBound(800);
        angleAllYAxis.setLowerBound(-410);
        
        sideAllGraph.setMinWidth(450);
        angleAllGraph.setMinWidth(450);
        sideAllGraph.setMaxWidth(450);
        angleAllGraph.setMaxWidth(450);
        
        
        
        mainInterface = new Interface(this);
        
        stage.setTitle("V.E.N.T:Q Control Room");
        
        
        
        Group graphics = new Group();
        
        GridPane root = new GridPane(); // Skapar en grid
        Scene mainScene = new Scene(root,1350,700); // Storlek är 1000x500 pixlar
        root.setAlignment(Pos.TOP_LEFT);
        root.setHgap(0); // Bestämmer horisontella och vertikala avstånd
        root.setVgap(0);
        root.setPadding(new Insets(10,25,25,10));
        stage.setScene(mainScene);
        mainScene.getStylesheets().add(GUI.class.getResource("GUI.css").toExternalForm()); // Hämtar CSS där bakgrundsbild och vissa textdefinitioner finns
        stage.show(); 
        
        
        connectButton.setText("    Anslut    "); // Om datorn ej ansluten till roboten används knappen för att ansluta,
        connectButton.setOnAction(new EventHandler<ActionEvent>() // annars använd den för att koppla från
        {
            @Override
            public void handle(ActionEvent event) // Funktion som sker när knappen trycks.
            {
                root.requestFocus();
                if (portConnected == "Ej ansluten") 
                {
                    setConnect = "Connect";
                    connectButton.setText("Koppla bort");
                }
                else
                {
                    resetButtons(); // Om ansluten, koppla från och ändra knapp, nollställ alla knapptryck
                    connectButton.setText("    Anslut    ");
                    setConnect = "Disconnect";
                }
            }
        });
        
        findLeakButton.setText(" Gå till läcka "); // När denna knapp trycks skickas ett meddelande till robot om att gå rill läcka
        findLeakButton.setOnAction(new EventHandler<ActionEvent>()
        {
            @Override
            public void handle(ActionEvent event)
            {
                root.requestFocus();
                try
                {
                    if (true)
                    {                   
                        switch(findLeak.getText())
                        {
                            case "1":
                            mainInterface.sendData((byte)0b11000000); // Skickar olika värden beroende på vald läcka
                            break;
                            case "2":
                            mainInterface.sendData((byte)0b11000001);
                            break;
                            case "3":
                            mainInterface.sendData((byte)0b11000010);
                            break;
                            case "4":
                            mainInterface.sendData((byte)0b11000011);
                            break;
                            case "5":
                            mainInterface.sendData((byte)0b11000100);
                            break;
                            default:
                            break;
                        }

                    }
                }
                catch (Exception e)
                {
                    System.err.println(e.toString());
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
                root.requestFocus();
                try
                {
                    if (!autonomusMode) // Om ej i autonomt läge, växla till autonomt läge genom att skicka rätt
                    {                   // värde till robot och ändra på text och knapp
                        mainInterface.sendData((byte)0b11010000); 
                        autonomusMode = true;
                        autonomusButton.setText("Avaktivera");
                        autonomusText.setText("Autonomt\nläge på");
                        resetButtons(); // Nollställer allt och skickar det till robot så den ej fortsätter 
                    }
                    else  // Om i autonomt läge, växla till manuellt
                    {
                        mainInterface.sendData((byte)0b11001000);
                        autonomusMode = false;
                        autonomusButton.setText(" Aktivera ");
                        autonomusText.setText("Autonomt\nläge av");
                        resetButtons(); // Nollställer allt och skickar det till robot så den ej fortsätter 
                    }
                }
                catch (Exception e)
                {
                    System.err.println(e.toString());
                }
            }
        });
        
        Button cleanButton = new Button(); // Knapp för att byta mellan manuellt och autonomt värde
        cleanButton.setText("Rensa grafer");
        cleanButton.setOnAction(new EventHandler<ActionEvent>()
        {
            @Override
            public void handle(ActionEvent event)
            {
               root.requestFocus();
               try
               {
                   resetGraphs();
               }
               catch(Exception e)
               {
                   System.err.println(e.toString());
               }
            }
        });
        
        Button resetListButton = new Button(); // Rensar alla beslutslistor
        resetListButton.setText("Reset");
        resetListButton.setOnAction(new EventHandler<ActionEvent>()
        {
            @Override
            public void handle(ActionEvent event)
            {
               root.requestFocus();
               try
               {
                   nodeList.setText("Noder:\n");
                   decisionList.setText("Styrbeslut:\n");
               }
               catch(Exception e)
               {
                   System.err.println(e.toString());
               }
            }
        });
 
        
       
        
        
        root.setOnKeyPressed(new EventHandler<KeyEvent>() // Eventhandler som hanterar när knapp trycks ner
        {
            public void handle(KeyEvent e)
            {
                if(e.getText().toLowerCase() == "r") // r används för "reset"
                {                                    // toLowerCase gör att capslock ej påverkar
                    resetButtons();
                }
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
                        case "g":
                            if (!gPressed)
                            {
                                gPressed = true;
                            }
                            break;
                        case "h":
                            if (!hPressed)
                            {
                                hPressed = true;
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
        });
        
        root.setOnKeyReleased(new EventHandler<KeyEvent>() // Eventhandler som hanterar när knapp släpps
        {                                                  // Knappuppsläpp fungerar omvänt som knapptryck,
            public void handle(KeyEvent e)                 // se funktion ovan
            {
                if (true)
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
                        case "g":
                            if (gPressed)
                            {
                                gPressed = false;
                            }
                            break;
                        case "h":
                            if (hPressed)
                            {
                                hPressed = false;
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
        
        root.setOnMouseClicked(new EventHandler<MouseEvent>()
        {
            public void handle(MouseEvent e)
            {
                root.requestFocus();
            }
        });
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
        
        timeIncrementText.setMinHeight(60);
        timeIncrementText.setMinWidth(80);
        stepLengthText.setMinHeight(60);
        stepLengthText.setMinWidth(80);
        stepHeightText.setMinHeight(60);
        stepHeightText.setMinWidth(80);
        heightText.setMinHeight(60);
        heightText.setMinWidth(80);
        xyWidthText.setMinHeight(60);
        xyWidthText.setMinWidth(80);
        kDistanceText.setMinHeight(60);
        kDistanceText.setMinWidth(80);
        kAngleText.setMinHeight(60);
        kAngleText.setMinWidth(80);
        decisionList.setMinWidth(160);
        decisionList.setMinHeight(680);
        nodeList.setMinWidth(160);
        nodeList.setMinHeight(680);
        
        comXX.setMinHeight(30);
        comXX.setMinWidth(80);
        findLeak.setMinHeight(30);
        findLeak.setMinWidth(40);
        
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
        decisionList.setMaxWidth(160);
        decisionList.setMaxHeight(680);
        nodeList.setMaxWidth(160);
        nodeList.setMaxHeight(680);
        
        
        timeIncrementText.setMaxHeight(60);
        timeIncrementText.setMaxWidth(80);
        stepLengthText.setMaxHeight(60);
        stepLengthText.setMaxWidth(80);
        stepHeightText.setMaxHeight(60);
        stepHeightText.setMaxWidth(80);
        heightText.setMaxHeight(60);
        heightText.setMaxWidth(80);
        xyWidthText.setMaxHeight(60);
        xyWidthText.setMaxWidth(80);
        kDistanceText.setMaxHeight(60);
        kDistanceText.setMaxWidth(80);
        kAngleText.setMaxHeight(60);
        kAngleText.setMaxWidth(80);
        
        comXX.setMaxHeight(30);
        comXX.setMaxWidth(80);
        findLeak.setMaxHeight(30);
        findLeak.setMaxWidth(40);
        
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
        
        
        nodeImageView.setImage(nodeImageArray[15]);
        root.add(nodeImageView,4,4,2,2);
        
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
        root.add(decisionList,12,0,2,16);
        root.add(nodeList,14,0,2,16);
        
        root.setMargin(decisionList, new Insets(0,10,0,10));
        root.setMargin(stepHeightText, new Insets(30,0,0,0));
        
        root.add(connectedText,8,0);
        root.add(connectButton,8,1); // lägger till knappar och deras texter
        root.add(comXX,9,1,2,1);
        
        
        root.add(autonomusText,8,6);
        root.add(autonomusButton,8,7);
        
        
        root.add(resetListButton,11,15);
        
        root.add(findLeakButton,8,15);
        root.add(findLeak,9,15);
        
        
        root.add(leftArrow,9,3); // Lägger till symboler för att visa rörelseriktning
        root.add(rightArrow,11,3);
        root.add(upArrow,10,2);
        root.add(downArrow,10,4);
        root.add(turnSymbol,10,3);
        
        turnSymbol.setBackground(blackBackground); // Ändrar utseende på symbolerna
        
        
        root.add(sideAllGraph,20,0,3,8);
        root.add(angleAllGraph,20,8,3,8);
        
        root.add(timeIncrementText,2,10,2,1);
        root.add(kDistanceText,0,11,2,1);
        root.add(kAngleText,4,11,2,1);
        root.add(stepLengthText,6,9,2,1);
        root.add(stepHeightText,8,8,1,1);
        root.add(xyWidthText,8,10,1,1);
        root.add(heightText,9,9,2,1);
        
        root.requestFocus();
        side1Text.setFocusTraversable(false);
        
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
        setText("decision");
        
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
