
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
import javafx.scene.text.Text;
import javafx.stage.Stage;
        
public class GUI extends Application
{
    private Interface test;
    
    Text vinkel1Text= new Text();
    Text vinkel2Text= new Text();
    Text vinkel3Text= new Text();
    Text vinkel4Text= new Text();
    Text vinkelTotalText= new Text();
    Text sida1Text = new Text();
    Text sida2Text = new Text();
    Text sida3Text = new Text();
    Text sida4Text = new Text();
    Text läckaText = new Text();
    Text nodText = new Text();
    
    
        
    void setAllText()
    {
        sida1Text.setText(Integer.toString(test.sidor[0]));
        sida2Text.setText(Integer.toString(test.sidor[1]));
        sida3Text.setText(Integer.toString(test.sidor[2]));
        sida4Text.setText(Integer.toString(test.sidor[3]));
        vinkel1Text.setText(Integer.toString(test.vinklar[0]));
        vinkel2Text.setText(Integer.toString(test.vinklar[1]));
        vinkel3Text.setText(Integer.toString(test.vinklar[2]));
        vinkel4Text.setText(Integer.toString(test.vinklar[3]));
        vinkelTotalText.setText(Integer.toString(test.vinklar[4]));
        läckaText.setText(test.läcka);
        nodText.setText(test.nod);
    }
    
    
    @Override
    public void start(Stage stage)
    {
        
        test = new Interface(this);
        
        stage.setTitle("V.E.N.T:Q Control Room");
        
        Button connectButton = new Button();
        connectButton.setText("Anslut");
        connectButton.setOnAction(new EventHandler<ActionEvent>()
        {
            @Override
            public void handle(ActionEvent event)
            {
                test.hittaportGUI("COM12");
                test.anslutGUI();
            }
        });
        test.hittaport();
        test.anslut();
        
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
        
        
        
        GridPane root = new GridPane();
        root.setAlignment(Pos.CENTER);
        root.setHgap(50);
        root.setVgap(10);
        root.setPadding(new Insets(25,25,25,25));
        stage.setScene(new Scene(root,700,500));
        stage.show();
        
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
        
        root.add(sida1Text,2,1);
        root.add(sida2Text,5,5);
        root.add(sida3Text,3,13);
        root.add(sida4Text,0,9);
        root.add(vinkel1Text,3,1);
        root.add(vinkel2Text,5,9);
        root.add(vinkel3Text,2,13);
        root.add(vinkel4Text,0,5);
        root.add(vinkelTotalText,3,5);
        root.add(läckaText,2,5);
        root.add(nodText,2,9);
        
        setAllText();
    }
    
            
    public static void main(String[] args)
    {
        GUI mainGUI = new GUI();
        launch(args);
    }
}
