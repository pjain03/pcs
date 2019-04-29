// AUTHORS: Annie Chen and Pulkit Jain
// PURPOSE: Scripts for the AP Cache Search Engine

function send_get_request() {
    url_txt = document.getElementsByName("url")[0].value;
    query_txt = document.getElementsByName("query")[0].value;
    if (url_txt == "") {
        url_txt = "comp112-02.cs.tufts.edu:9085";
        alert("Proxy URL defaulting to " + url_txt);
    }
    if (query_txt == "") {
        alert("ABORT: Query was empty!");
    } else {
        $("#outer").css("justify-content", "flex-start");
        $("#search-bar").css({
            "width": "100%",
            "padding-bottom": "1%",
            "margin-bottom": "1%",
            "border-bottom": "1px solid #d3d3d3",
            "flex-direction": "row"
        });
        $("h1").css("margin", "0");
        $("#heading").css("margin-right", "1%");
        $.get({
            url: url_txt,
            data: {
                query: query_txt
            },
            success: function (response) {
                $("#error").css("display", "none");
                $("#results").html(response);
                $("#results").css("display", "flex");
            },
            failure: function (xhr, status) {
                $("#results").css("display", "none");
                $("#error").css("display", "flex");
            },
            error: function (xhr, status) {
                $("#results").css("display", "none");
                $("#error").css("display", "flex");
            }
        });
    }
}

$("#search").submit(function (e) {
    // Stop the form from reloading the page
    e.preventDefault();
    send_get_request();
});

$("#search").keydown(function (e) {
    // Enter key should submit the form
    if (e.which == 13) {
        send_get_request();
    }
});

$("#submit").click(function (e) {
    send_get_request();
});


