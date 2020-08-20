<?php
//20-08-2020
//started on 09-07-2019
// La app di Heroku si puo richiamare da browser con
//			https://rele4lamps1.herokuapp.com/

/////@lamp1tgbot			 ESP_lanterna

/*API key = 666665671:AAG1ZOr7L9aajwN7mkFUkspednZImUWapXs

da browser request ->   https://api.telegram.org/bot666665671:AAG1ZOr7L9aajwN7mkFUkspednZImUWapXs/getMe
           answer  <-   {"ok":true,"result":{"id":666665671,"is_bot":true,"first_name":"lamp1tg","username":"lamp1tgbot"}}

riferimenti:
https://gist.github.com/salvatorecordiano/2fd5f4ece35e75ab29b49316e6b6a273
https://www.salvatorecordiano.it/creare-un-bot-telegram-guida-passo-passo/
*/

//------passaggio da getupdates a  WEBHOOK
//da browser request ->   https://api.telegram.org/bot666665671:AAG1ZOr7L9aajwN7mkFUkspednZImUWapXs/setWebhook?url=https://rele4lamps1.herokuapp.com/execute.php
//					 answer  <-   {"ok":true,"result":true,"description":"Webhook was set"}
//          From now If the bot is using getUpdates, will return an object with the url field empty.
//------passaggio da webhook a  GETUPDATES
//da browser request ->   https://api.telegram.org/bot666665671:AAG1ZOr7L9aajwN7mkFUkspednZImUWapXs/setWebhook?url=
//					 answer  <-   {"ok":true,"result":true,"description":"Webhook was deleted"}

$content = file_get_contents("php://input");
$update = json_decode($content, true);

if(!$update){
  exit;
}


function clean_html_page($str_in){
	$startch = strpos($str_in,"er><h2>") + 1 ;									//primo carattere utile da estrarre
	$endch = strpos($str_in," </a></h2><foot");									//ultimo carattere utile da estrarre
	$str_in = substr($str_in,$startch,$endch - $startch);				// substr(string,start,length)
	$str_in = str_replace("<a href='?a="," ",$str_in);
	$str_in = str_replace("r><h2>"," ",$str_in);
	$str_in = str_replace(" </a></h2><h2>"," ",$str_in);
	return $str_in;
}

$message = isset($update['message']) ? $update['message'] : "";
$messageId = isset($message['message_id']) ? $message['message_id'] : "";
$chatId = isset($message['chat']['id']) ? $message['chat']['id'] : "";
$firstname = isset($message['chat']['first_name']) ? $message['chat']['first_name'] : "";
$lastname = isset($message['chat']['last_name']) ? $message['chat']['last_name'] : "";
$username = isset($message['chat']['username']) ? $message['chat']['username'] : "";
$date = isset($message['date']) ? $message['date'] : "";
$text = isset($message['text']) ? $message['text'] : "";

// pulisco il messaggio ricevuto togliendo eventuali spazi prima e dopo il testo
$text = trim($text);
// converto tutti i caratteri alfanumerici del messaggio in minuscolo
$text = strtolower($text);

header("Content-Type: application/json");

//ATTENZIONE!... Tutti i testi e i COMANDI contengono SOLO lettere minuscole
$response = '';

if(strpos($text, "/start") === 0 || $text=="ciao" || $text == "help"){
	$response = "Ciao $firstname, benvenuto! \n List of commands : 
	/tlc_on   -> telecamera accesa
	/tlc_off  -> telecamera spenta
	/lant_on  -> Lanterna accesa	
	/lant_off -> Lanterna spenta
	/stato 		-> Stato rele     \n/verbose -> parametri del messaggio";
}

//<-- Comandi al rele GPIO1
elseif(strpos($text,"tlc_on")){
	$resp = file_get_contents("http://dario95.ddns.net:28082/?a=3");
	$response = clean_html_page($resp);
}
elseif(strpos($text,"tlc_off")){
	$resp = file_get_contents("http://dario95.ddns.net:28082/?a=2");
	$esponse = clean_html_page($resp);
}
//<-- Comandi al rele GPIO2
elseif(strpos($text,"lant_on")){
	$resp = file_get_contents("http://dario95.ddns.net:28082/?a=1");
	$response = clean_html_page($resp);
}
elseif(strpos($text,"lant_off")){
	$resp = file_get_contents("http://dario95.ddns.net:28082/?a=0");
	$response = clean_html_page($resp);
}
//<-- Lettura stato dei rele 
elseif(strpos($text,"stato")){
	$resp = file_get_contents("http://dario95.ddns.net:28082");
	$response = clean_html_page($resp);
}


//<-- Manda a video la risposta completa
elseif($text=="/verbose"){
	$response = "chatId ".$chatId. "   messId ".$messageId. "  user ".$username. "   lastname ".$lastname. "   firstname ".$firstname ;		
	$response = $response. "\n\n Heroku + dropbox gmail.com";
}
else
{
	$response = "Unknown command!";			//<---Capita quando i comandi contengono lettere maiuscole
}
// Gli EMOTICON sono a:     http://www.charbase.com/block/miscellaneous-symbols-and-pictographs
//													https://unicode.org/emoji/charts/full-emoji-list.html
//													https://apps.timwhitlock.info/emoji/tables/unicode
// la mia risposta è un array JSON composto da chat_id, text, method
// chat_id mi consente di rispondere allo specifico utente che ha scritto al bot
// text è il testo della risposta
$parameters = array('chat_id' => $chatId, "text" => $response);
$parameters["method"] = "sendMessage";
// imposto la keyboard
$parameters["reply_markup"] = '{ "keyboard": [
["/lant_on \ud83d\udd34", "/tlc_on \ud83d\udd34"],
["/lant_off \ud83d\udd35", "/tlc_off \ud83d\udd35"],
["/stato \u2753"]],
 "resize_keyboard": true, "one_time_keyboard": false}';
// converto e stampo l'array JSON sulla response
echo json_encode($parameters);
?>