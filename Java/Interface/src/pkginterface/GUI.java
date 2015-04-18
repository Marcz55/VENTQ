
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

import javafx.scene.input.KeyCode;
import javafx.scene.input.KeyEvent;
        
public class GUI extends Application
{
    private Interface mainInterface;
    private boolean autonomusMode = false;
    private int turnDirection = 0; // Vridning åt höger innebär positiv vridning
    private int upMovement = 0; // Rörelse uppåt är positiv
    private int rightMovement = 0; // Rörelse åt höger är positiv
    
    Text angle1Text= new Text();
    Text angle2Text= new Text();
    Text angle3Text= new Text();
    Text angle4Text= new Text();
    Text angleTotalText= new Text();
    Text side1Text = new Text();
    Text side2Text = new Text();
    Text side3Text = new Text();
    Text side4Text = new Text();
    Text leakText = new Text();
    Text nodeText = new Text();
    Text connectedText = new Text();
    Text upArrow = new Text();
    Text downArrow = new Text();
    Text leftArrow = new Text();
    Text rightArrow = new Text();
    Text turnSymbol = new Text();
    Text autonomusText = new Text("Autonomt läge av");
    boolean wPressed = false;
    boolean aPressed = false;
    boolean sPressed = false;
    boolean dPressed = false;
    boolean qPressed = false;
    boolean ePressed = false;
    
    
    
    

    
    void setMovement()
    {
        switch(upMovement)
        {
            case 0:
                upArrow.setFill(Color.BLACK);
                downArrow.setFill(Color.BLACK);
                break;
            case 1:
                upArrow.setFill(Color.LAWNGREEN);
                downArrow.setFill(Color.BLACK);
                break;
            case -1:
                upArrow.setFill(Color.BLACK);
                downArrow.setFill(Color.LAWNGREEN);
                break;
            default:
                upArrow.setFill(Color.BLACK);
                downArrow.setFill(Color.BLACK);
                break;
                
        }
        switch(rightMovement)
        {
            case 0:
                rightArrow.setFill(Color.BLACK);
                leftArrow.setFill(Color.BLACK);
                break;
            case 1:
                rightArrow.setFill(Color.LAWNGREEN);
                leftArrow.setFill(Color.BLACK);
                break;
            case -1:
                rightArrow.setFill(Color.BLACK);
                leftArrow.setFill(Color.LAWNGREEN);
                break;
            default:
                rightArrow.setFill(Color.BLACK);
                leftArrow.setFill(Color.BLACK);
                break; 
                
        }
        
        switch(turnDirection)
        {
            case 0:
                turnSymbol.setText("");
                break;
            case 1:
                turnSymbol.setText("H");
                break;
            case -1:
                turnSymbol.setText("V");
                break;
            default:
                turnSymbol.setText("");
                break;
        }
    }
        
    void setAllText()
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
    
    
    @Override
    public void start(Stage stage)
    {
        
        mainInterface = new Interface(this);
        
        stage.setTitle("V.E.N.T:Q Control Room");
        
        Button connectButton = new Button();
        connectButton.setText("    Anslut    ");
        connectButton.setOnAction(new EventHandler<ActionEvent>()
        {
            @Override
            public void handle(ActionEvent event)
            {
                if (mainInterface.portConnected == "Ej ansluten")
                {
                    mainInterface.hittaportGUI("COM12"); // COM** är olika på olika datorer
                    if (mainInterface.comport != null)
                    {
                        mainInterface.anslut();
                    }
                    if(mainInterface.portConnected == "Ansluten")
                    {
                        connectButton.setText("Koppla bort");
                    }
                }
                else
                {
                    mainInterface.flush();
                    connectButton.setText("    Anslut    ");
                }
            }
        });
        
        Button autonomusButton = new Button();
        autonomusButton.setText(" Aktivera ");
        autonomusButton.setOnAction(new EventHandler<ActionEvent>()
        {
            @Override
            public void handle(ActionEvent event)
            {
                if (!autonomusMode)
                {
                    mainInterface.skickainput("A"); // Skickar A för autonomt
                    System.out.println("A");
                    autonomusMode = true;
                    autonomusButton.setText("Avaktivera");
                    autonomusText.setText("Autonomt läge på");
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
                else
                {
                    mainInterface.skickainput("M"); // Skickar M för manuellt
                    System.out.println("M");
                    autonomusMode = false;
                    autonomusButton.setText(" Aktivera ");
                    autonomusText.setText("Autonomt läge av");
                }
            }
        });
        
        Label vinkel1 = new Label("Vinkel 1:");
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
        
        
        GridPane root = new GridPane();
        root.setAlignment(Pos.CENTER);
        root.setHgap(50);
        root.setVgap(10);
        root.setPadding(new Insets(25,25,25,25));
        stage.setScene(new Scene(root,1000,500));
        stage.show();
        
        root.setOnKeyPressed(new EventHandler<KeyEvent>()
        {
            public void handle(KeyEvent e)
            {
                if (!autonomusMode)
                {
                    switch (e.getText().toLowerCase()) // toLowerCase om capslock nedtryckt
                    {
                        case "w":
                            if (!wPressed)
                            {
                                upMovement ++;
                                wPressed = true;
                            }
                            break;
                        case "a":
                            if (!aPressed)
                            {
                                rightMovement --;
                                aPressed = true;
                            }
                            break;
                        case "s":
                            if (!sPressed)
                            {
                                upMovement --;
                                sPressed = true;
                            }
                            break;
                        case "d":
                            if (!dPressed)
                            {
                                rightMovement ++;
                                dPressed = true;
                            }
                            break;
                        case "q":
                            if (!qPressed)
                            {
                                turnDirection --;
                                qPressed = true;
                            }
                            break;
                        case "e":
                            if (!ePressed)
                            {
                                turnDirection ++;
                                ePressed = true;
                            }
                            break;
                        default:
                            break;
                    }
                    setMovement();
                    System.out.print("Upp: ");
                    System.out.print(upMovement);
                    System.out.print(", Höger: ");
                    System.out.print(rightMovement);
                    System.out.print(" , Vridning: ");
                    System.out.println(turnDirection);
                }
            }
        });
        
        root.setOnKeyReleased(new EventHandler<KeyEvent>()
        {
            public void handle(KeyEvent e)
            {
                if (!autonomusMode)
                {
                    switch (e.getText().toLowerCase())
                    {
                        case "w":
                            if (wPressed)
                            {
                                upMovement --;
                                wPressed = false;
                            }
                            break;
                        case "a":
                            if (aPressed)
                            {
                                rightMovement ++;
                                aPressed = false;
                            }
                            break;
                        case "s":
                            if (sPressed)
                            {
                                upMovement ++;
                                sPressed = false;
                            }
                            break;
                        case "d":
                            if (dPressed)
                            {
                                rightMovement --;
                                dPressed = false;
                            }
                            break;
                        case "q":
                            if (qPressed)
                            {
                                turnDirection ++;
                                qPressed = false;
                            }
                            break;
                        case "e":
                            if (ePressed)
                            {
                                turnDirection --;
                                ePressed = false;
                          }
                            break;
                        default:
                            break;
                    }
                    setMovement();
                    System.out.print("Upp: ");
                    System.out.print(upMovement);
                    System.out.print(", Höger: ");
                    System.out.print(rightMovement);
                    System.out.print(" , Vridning: ");
                    System.out.println(turnDirection);
                }
            }
        });
        
        
        
        
        root.add(sida1,2,0);
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
        
        root.add(side1Text,2,1);
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
        
        root.add(connectButton,7,13);
        root.add(connected,7,11);
        root.add(connectedText,7,12);
        
        root.add(autonomusText,7,0);
        root.add(autonomusButton,7,1);
        
        root.add(leftArrow,8,8);
        root.add(rightArrow,10,8);
        root.add(upArrow,9,5);
        root.add(downArrow,9,10);
        root.add(turnSymbol,9,8);
        
        upArrow.setText("^");
        downArrow.setText("v");
        leftArrow.setText("<");
        rightArrow.setText(">");
        turnSymbol.setText("");
        
        
        setAllText();
    }
    
            
    public static void main(String[] args)
    {
        GUI mainGUI = new GUI();
        launch(args);
    }
}
