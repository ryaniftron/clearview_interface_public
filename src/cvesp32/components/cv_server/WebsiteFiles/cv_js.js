console.log("cv_js_loaded")

$(document).ready(function(){
    $("select").val("");
    $("select").change( field_change); //attach the field change event to field_change
});

function field_change() {
    $("#"+this.id).submit()
}

function process_error_reply(domEl, k, v_new){
    $("#" + k)
        .prop('disabled', false)
        .parent()
        .removeClass()
        .addClass('error');    
    
    var error_id = "#error_" + k;
        $(error_id)
            .html("Error: "+ v_new)
            .slideDown();
        
}

function receive_data(msg) {
    for (var k in msg) {
        var v = msg[k]
        console.log("Received kv pair:" + k + "-> " + msg[k])
        if (k == "req_report") {
            document.getElementById(k).innerHTML="Response:" + "'" + msg[k]+ "'";
        }else if (k == "send_cmd"){
             continue
        } else {
            var domEls = Array(document.getElementById(k));
            var domCls = document.getElementsByClassName(k);
            if (domCls.length >= 1) {
                var loop_els = domCls;
                // console.log("Using class looper")
            } else {
                var loop_els = domEls;
                // console.log("Using 1 element looper")
            }

            for (let domEl of loop_els){
                // var jqEl = $(domEl);
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
                        domEl.value = v
                        $("#" + k)
    						.prop('disabled', false)
    						.parent()
    						.removeClass()
    						.addClass('success');
    					$("#error_" + k).slideUp();
                    } else if (domElTagName == "INPUT"){ //text got some reply
                        if (v.includes("error")) { // error reply
                            $("#" + k)
							    .prop('disabled', false)
							    .parent()
							    .removeClass()
							    .addClass('error');
                            process_error_reply(domEl, k, v);
                        } else { //success text reply
                            domElOpt.value = v
                            $("#" + k)
							    .prop('disabled', false)
							    .parent()
							    .removeClass()
							    .addClass('success');
                            $("#error_" + k).slideUp()
                        } 
                    } else {
                        if (v.includes("error")) {
                            $("#" + k)
							    .prop('disabled', false)
							    .parent()
							    .removeClass()
							    .addClass('error');
                            process_error_reply(domEl, k, v);
                        } else {
                            if (k == "seat") { 
                                domEl.innerText = (parseInt(v)+1).toString(10);
                            }
                            else {
                                domEl.innerText = v;
                            }
                            $("#" + k)
							    .prop('disabled', false)
							    .parent()
                                .removeClass()
                                .addClass('success')
                            $("#error_" + k).slideUp()
                        }
                    } 
                } else {
                    console.log("Process error reply for id " + k);
                    // Fade: https://stackoverflow.com/questions/26936811/css-transition-fade-background-color-resetting-after
                    process_error_reply(domEl, k,v);
                }
            };
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
                
                if (this.name != "send_cmd" && this.name != "req_report"){
                    $("#" + this.name)
                        .prop('disabled', true)
                        .parent()
                        .removeClass()
                        .addClass('working')
                }
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