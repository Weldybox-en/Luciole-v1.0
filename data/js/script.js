
var on;

var red;
var green;
var blue;

var rangeDefine;
var pickColor = 0;

//var previousOn = false; //Précédent état de on
 var previousColor;


/*---------------------------------------------------------
Connexion initialization function
---------------------------------------------------------*/
function init(){
  if(getCookie("onOFf") == "on"){
    document.getElementById("onOFf").checked = true;
    on = true;
  }else if(getCookie("onOFf") == "off"){
    document.getElementById("onOFf").checked = false;
    on = false;
  }

  if(getCookie("smartlight") == "off"){
    document.getElementById("smartLightSwitch").checked = false;

  }else if(getCookie("smartlight") == "on"){
    document.getElementById("smartLightSwitch").checked = true;
    document.getElementById("onOFf").checked = false;
    on = false;
    //checkSLCheckProcess();
  }

  Papa.parse("saveA.csv", {
    header: false,
    download: true,
    delimiter: ";",
    dynamicTyping: true,
    complete: function(results) {
      document.getElementById("sous-menu3").style.background = "rgb(" + results.data[0][0] + ")";
    }
  });
}



/*---------------------------------------------------------
Cookie decode's function
---------------------------------------------------------*/
function getCookie(cname) {
  var name = cname + "=";
  var decodedCookie = decodeURIComponent(document.cookie);
  var ca = decodedCookie.split(';');
  for(var i = 0; i <ca.length; i++) {
    var c = ca[i];
    while (c.charAt(0) == ' ') {
      c = c.substring(1);
    }
    if (c.indexOf(name) == 0) {
      return c.substring(name.length, c.length);
    }
  }
  return "";
}

/*---------------------------------------------------------
Objects relative to the websocket
---------------------------------------------------------*/
var connection = new WebSocket('ws://' + location.hostname + ':81/',['arduino']);
connection.onmessage = function(event){
  console.log(event.data);
  colorPicker.color.rgbString = event.data;
};
connection.onerror = function (error) {
  console.log('WebSocket Error ', error);
};
connection.onclose = function () {
  console.log('WebSocket connection closed');
};
  
function chooseColorRange(i){
  console.log("fonction de changement de background color");
  pickColor = i;
}


window.onload=function(){
  var bouton = document.getElementById('btnMenu');
  var nav = document.getElementById('nav');
  bouton.onclick = function(e){
      if(nav.style.display=="block"){
          nav.style.display="none";
      }else{
          nav.style.display="block";
      }
  };
};


/*---------------------------------------------------------
Function that show saved colors to the user
---------------------------------------------------------*/
function displaySave(results){
  if(results.data[0][0] == null){
    var number = 1;
  }else{var number =0;}
  console.log(results);
  var count = 0;
  results.data[number].forEach(element => {
    var id= 'carre' + String(count+1);
    console.log(element[count]);

    var backgroundcolor = "rgb(" + element + ")";
    document.getElementById(id).style.background = backgroundcolor;
    count++;
  });
}

/*---------------------------------------------------------
Functino that read .csv content
---------------------------------------------------------*/
var position = [];
function chooseSave(filename){
  Papa.parse(filename, {
    header: false,
    download: true,
    delimiter: ";",
    dynamicTyping: true,
    complete: function(results) {
      if(filename == "save.csv"){
      displaySave(results); 
      }else{
        console.log(results);

        var count = 0;
        results.data[0].forEach(element => {
          var backgroundcolor = "rgb(" + element + ")";
          document.getElementById("sous-menu"+((count+1).toString(10))).style.background = backgroundcolor;
          count++;
        });
      }
    }
  });
}

/*---------------------------------------------------------
Selection colorpick color according to the .csv files
---------------------------------------------------------*/
function setSaveColor(id){
  Papa.parse('save.csv', {
    header: false,
    download: true,
    delimiter: ";",
    dynamicTyping: true,
    complete: function(results) {
      if(results.data[0][0] == null){
        var number = 1;
      }else{var number =0;}
      var tableau = results.data[number];
      colorPicker.color.rgbString = "rgb("+ tableau[id-1] +")";
    }
  });
}

/*---------------------------------------------------------
Shutdown lights function
---------------------------------------------------------*/
function sendOff(){
  connection.send("R0");
  connection.send("G0");
  connection.send("B0");
}

/*---------------------------------------------------------
Function that send the colors to save
---------------------------------------------------------*/
function saving(){
  connection.send("sR"+red);
  connection.send("sG"+green);
  connection.send("sB"+blue);

}

/*---------------------------------------------------------
Function that refresh colors every 0.5 seconds
---------------------------------------------------------*/
setInterval(function() {
  if(on){
    connection.send("R"+red);
    connection.send("G"+green);
    connection.send("B"+blue);

  }
}, 500);

/*---------------------------------------------------------
Smart lights proceedings (when it is turn on)
---------------------------------------------------------*/
function checkSLCheckProcess(){ 
  if(document.getElementById("onOFf").checked){ //If the free mode is actived
    document.cookie = "previousC =" + colorPicker.color.rgbString;
    //previousColor = colorPicker.color.rgbString;
    document.getElementById("onOFf").checked = false; //turn off free mode
    on = false;
    document.cookie = "previousOn =true"; //When on/off button is on, let's set previousOn true
  }else{
    document.cookie = "previousOn =false"; //When on/off button is off, let's set previousOn false
  }
  connection.send("#1");
}

/*---------------------------------------------------------
Smart lights proceedings (when it is turn off)
---------------------------------------------------------*/
function checkSLUncheckProcess(){
  if(getCookie("previousOn") == "true"){ //If previous state "on"
    colorPicker.color.rgbString = getCookie("previousC");
    document.getElementById("onOFf").checked = true; //Actual state of the switch at "on"
    
    on = true;
    connection.send("#2");
  }else{

    sendOff();
    connection.send("#2");
  }
  document.cookie = "previousOn =true"; //Set the cookie previousOn at true

  setInterval.println("off");
}

function saveAlarme(){
  console.log("ok");
  //connection.send("sA");
  var couleurs = ["sAR","sAG","sAB"];

  for(var i=0;i<3;i++){
    console.log(couleurs[i] + ((((((document.getElementById("sous-menu3").style.background).split("("))[1]).split(")"))[0]).split(","))[i]);

    connection.send(couleurs[i] + ((((((document.getElementById("sous-menu3").style.background).split("("))[1]).split(")"))[0]).split(","))[i]);

  }
  
}

//Function that managed interactions with the user
document.addEventListener('DOMContentLoaded', function () {
  var SmartLight = document.querySelector('input[name=SmartLight]');
  var checkbox = document.querySelector('input[type="checkbox"]');
  var Reglage = document.querySelector('input[name=menu-open]');
  var SaveMenu = document.querySelector('input[name=SaveMenu]');


  //Color's range selection of the smart light option
  Reglage.addEventListener('change', function () {
    if(Reglage.checked){
      if(SaveMenu.checked){
        document.getElementById("menu-toggle").checked = false;
      }
      chooseSave("saveS.csv");
      rangeDefine = true;
    }else{
      rangeDefine = false;
      pickColor = 0;

      var couleurs = ["sTR","sTG","sTB"];

      for(var i=0;i<3;i++){
        //console.log(couleurs[i] + ((((((document.getElementById("sous-menu2").style.background).split("("))[1]).split(")"))[0]).split(","))[i]);

        connection.send(couleurs[i] + ((((((document.getElementById("sous-menu1").style.background).split("("))[1]).split(")"))[0]).split(","))[i]);

      }
      for(var i=0;i<3;i++){
      connection.send(couleurs[i] + ((((((document.getElementById("sous-menu2").style.background).split("("))[1]).split(")"))[0]).split(","))[i]);
      }
    }
  });

  //Display saved colors actions.
  checkbox.addEventListener('change', function () {
    if (checkbox.checked) {
      document.cookie = "onOFf=on; expires=Thu, 18 Dec 2020 12:00:00 UTC";
      on = true;
    } else {
      document.cookie = "onOFf=off; expires=Thu, 18 Dec 2020 12:00:00 UTC";
      sendOff();
      on = false;
    }
  });

  /*---------------------------------------------------------
  When the user act on the smart light switch
  ---------------------------------------------------------*/
  SmartLight.addEventListener('change', function () {
    
    if (SmartLight.checked) {
      document.cookie = "smartlight=on; expires=Thu, 18 Dec 2020 12:00:00 UTC";
      checkSLCheckProcess();
    } else {
      document.cookie = "smartlight=off; expires=Thu, 18 Dec 2020 12:00:00 UTC"; 
      checkSLUncheckProcess();
    }
  });

  //Smartlight saved colors displays
  SaveMenu.addEventListener('change', function () {
    if(SaveMenu.checked){
      if(Reglage.checked){
        document.getElementById("menu-open").checked = false;
      }
    }else{
      console.log("off");
    }
  });
});

var colorPicker = new iro.ColorPicker(".colorPicker", {
  width: 350,
  borderWidth: 1,
  color:getCookie("cursor"),// "#ffff",
  borderColor: "#fff",
});

 colorPicker.on(["color:init", "color:change"], function(color){
    red = ((color.rgbString).split(',')[0]).substring(4, 7);
    green = (color.rgbString).split(',')[1];
    var longueur =  ((color.rgbString).split(',')[2]).length;
    blue = (((color.rgbString).split(',')[2]).substring(0,longueur-1));

    document.cookie = "cursor="+"rgb("+red+","+green+","+blue+"); expires=Thu, 18 Dec 2020 12:00:00 UTC";

    if(rangeDefine && pickColor){
      var backgroundcolor = "rgb("+red+","+green+","+blue+")";
      document.getElementById("sous-menu"+pickColor).style.background = backgroundcolor;
    }else if(pickColor == 3){
      var backgroundcolor = "rgb("+red+","+green+","+blue+")";
      document.getElementById("sous-menu"+pickColor).style.background = backgroundcolor;
    }
});


/*Init function*/
init();
