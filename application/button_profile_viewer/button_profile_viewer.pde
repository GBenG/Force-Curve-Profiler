//Includes
import     java.io.BufferedWriter;
import     java.io.FileWriter;
import     processing.serial.*;

//Objects
Serial_port serial = new Serial_port();

ProgressBar progressBar;
Chrat       press, relis, ref_prs, ref_rel;

void setup() 
{
  size( 1200, 850 );
  
  serial.update();
  
  //Progress bar
  progressBar = new ProgressBar(this, 0, 0, width, 10, color(bgcolor), color(txcolor), true);
  press = new Chrat( 10, 10, width-20, height-100, color(#00a2ff), 2, true);
  relis = new Chrat( 10, 10, width-20, height-100, color(#ff9a00), 2, true);
  
  ref_prs = new Chrat( 10, 10, width-20, height-100, color(#00a2ff,50), 1, true);
  ref_rel = new Chrat( 10, 10, width-20, height-100, color(#ff9a00,50), 1, true);
  
  ref_prs.upload_data("ref_scan.txt", "PRESS");
  ref_rel.upload_data("ref_scan.txt", "RELEASE");

}



void draw() 
{
  background(bgcolor);
//-------------------------------------------------------- 
  control_watermark();
  control_group("LEFT", 0, height);
//--------------------------------------------------------
  relis.display();
  press.display();
  ref_rel.display();
  ref_prs.display();
//--------------------------------------------------------
}

void mouseWheel(MouseEvent event) {
  float e = event.getCount();
  println(e);
}

void keyPressed() {

}
