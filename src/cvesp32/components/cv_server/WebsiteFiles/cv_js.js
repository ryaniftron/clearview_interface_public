console.log("cv_js_loaded")

$(document).ready(function(){
    $("select").change( field_change); //attach the field change event to field_change
});

function field_change() {
    $("#"+this.id).submit()
    //.css("background", "rgb(200,200,0)").
}

function process_error_reply(domEl, k, v_new){
        // domEl.classList.add("style_error")
        // if (data['status'] == 'fail') {
		// 		$(body).addClass('no-cv-response');
        var error_id = "#error_" + k;
		$(error_id).html("Error: "+ v_new)
            .css({
                "color":"#800",
                "background":"#fcc"
            })
            .slideDown();
				
		// 	} else {
		// 		$.each(data, function (key, val) {
		// 			$('#' + key).val(val);
		// 		});
		// 	}
}

function receive_data(msg) {
    for (var k in msg) {
        var v = msg[k]
        // console.log(k + "-> " + msg[k])
        if (k == "req_report") {
            document.getElementById(k).innerHTML="Response:" + "'" + msg[k]+ "'"
        } else {
            var domEl = document.getElementById(k);
            console.log(k)
            var jqEl = $(domEl);
            var domElTagName = domEl.tagName
            if (domElTagName == "INPUT"){
                var domElOpt = domEl;
            } else if (domElTagName == "SELECT") {
                var domElOpt = domEl.querySelector('[value="' + v + '"]');
            } else {
                var domElOpt = domEl
            } 

            
            if (domElOpt != null) {
                if (domElTagName == "SELECT") { //select got a valid reply
                    domElOpt.value = v
                    $("#" + k).css("background","rgb(0,200,0)");
                    $("#error_" + k).slideUp().hide();
                } else if (domElTagName == "INPUT"){ //text got some reply
                    if (v.includes("error")) { // error reply
                        $("#" + k).css("background","rgb(200,0,0)");
                        process_error_reply(domEl, k, v);
                    } else { //success text reply
                        domElOpt.value = v
                        $("#" + k).css("background","rgb(0,200,0)");
                        $("#error_" + k).slideUp().hide();
                    } 
                } else {
                    if (v.includes("error")) {
                        $("#" + k).css("background","rgb(200,0,0)");
                        process_error_reply(domEl, k, v);
                    } else {
                        domEl.innerText = v
                        $("#" + k).css("background","rgb(0,200,0)");
                        $("#error_" + k).slideUp().hide();
                    }
                } 
            } else {
                console.log("Process error id " + k);
                // Fade: https://stackoverflow.com/questions/26936811/css-transition-fade-background-color-resetting-after
                process_error_reply(domEl, k,v);
            }
            // el.innerHTML=k[msg]
        }
    }
}

    
function send_form(jsondata){
    console.log("Sending JSON form to server:" + jsondata)  
    var request = $.ajax({
        type: "POST",
        url: '/settings',
        dataType: 'json',
        data: jsondata,
        contentType: "application/json",
        processData: false,
        success: receive_data
    })
    .done(function(data){
        console.log("Done")
    })
    .fail(function(){
        console.log("Fail Submit")
    });
}

$(document).on('submit', 'form', function (event){
    event.preventDefault();
    var object = {};
    $(this.elements).each(function() {
        $(this).each(function() {
            if (this.name != "") {
                object[this.name] = this.value
                $("#" + this.name).css("background","rgb(200,200,0)")
            }
        });
    });
    object = JSON.stringify(object);
    send_form(object);
}); 

function getAllJSON(json_req){
    var xhr = new XMLHttpRequest();
    var requestURL = "/settings";
    xhr.open('POST', requestURL, true);

    //Send the proper header information along with the request
    xhr.setRequestHeader("Content-Type", "application/json");

    xhr.onreadystatechange = function() { // Call a function when the state changes.
        if (this.readyState === XMLHttpRequest.DONE && this.status === 200) {
            var response = JSON.parse(xhr.responseText);
            receive_data(response)
        }
    }

    xhr.send(JSON.stringify(json_req));
}