<?php
//09-07-2019
//started on 09-07-2019
// La app di Heroku si puo richiamare da browser con
//			https://rele4lamps1.herokuapp.com/


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

if(!$update)
{
  exit;
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
	/r10 -> GPIO1 LOW  /r11 -> GPIO1 HIGH 
	/r20 -> GPIO2 LOW  /r21 -> GPIO2 HIGH 
	/status  -> Lettura     \n/verbose -> parametri del messaggio";
}

//<-- Comandi al rele GPIO1
elseif(strpos($text,"r10")){
	$resp = substr(file_get_contents("http://dario95.ddns.net:28082/gpio1/0"),29);
	$resp1 = substr($resp,0,-15);
	$resp2 = substr($resp1,0,9);
	$resp3 = substr($resp1,26);
	$response = $resp2.$resp3;
}
elseif(strpos($text,"r11")){
	$resp = substr(file_get_contents("http://dario95.ddns.net:28082/gpio1/1"),29);
	$resp1 = substr($resp,0,-15);
	$resp2 = substr($resp1,0,9);
	$resp3 = substr($resp1,26);
	$response = $resp2.$resp3;
}
//<-- Comandi al rele GPIO2
elseif(strpos($text,"r20")){
	$resp = substr(file_get_contents("http://dario95.ddns.net:28082/gpio2/0"),29);
	$resp1 = substr($resp,0,-15);
	$resp2 = substr($resp1,0,9);
	$resp3 = substr($resp1,26);
	$response = $resp2.$resp3;
}
elseif(strpos($text,"r21")){
	$resp = substr(file_get_contents("http://dario95.ddns.net:28082/gpio2/1"),29);
	$resp1 = substr($resp,0,-15);
	$resp2 = substr($resp1,0,9);
	$resp3 = substr($resp1,26);
	$response = $resp2.$resp3;
}
//<-- Comando Total OFF
elseif(strpos($text,"roff")){
	$resp = substr(file_get_contents("http://dario95.ddns.net:28082/gpio3/0"),29);
	$response = substr($resp,0,-15);
}
//<-- Comando Total ON
elseif(strpos($text,"ron")){
	$resp = substr(file_get_contents("http://dario95.ddns.net:28082/gpio3/1"),29);
	$response = substr($resp,0,-15);
}

//<-- Lettura stato del rele GPIO2
elseif(strpos($text,"stato")){
	$response = file_get_contents("http://dario95.ddns.net:20082/gpio2?");
}
//<-- reset modulo
elseif(strpos($text,"reset")){
	$response = file_get_contents("http://dario95.ddns.net:28082/rst");
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
["/r21 \ud83d\udd34", "/r11 \ud83d\udd34"],
["/r20 \ud83d\udd35", "/r10 \ud83d\udd35"],
["/ron \ud83d\udd34", "/roff \ud83d\udd35"],
["/stato \u2753", "/reset"]],
 "resize_keyboard": true, "one_time_keyboard": false}';
// converto e stampo l'array JSON sulla response
echo json_encode($parameters);
?>