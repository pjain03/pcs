// AUTHORS: Annie Chen and Pulkit Jain
// PURPOSE: Scripts for the AP Cache Search Engine

function get_cached(url) {
    proxy = document.getElementsByName("url")[0].value;
    $.get({
        url: proxy,
        data: {
            get_cache: url
        },
        success: function (response) {
            $("#error").css("display", "none");
            $("results").css("display", "none");
            $("#viewer #url").html("<a href=\"" + url + "\">" + url + "</a>");
            $("#viewer #content").html(response);
            $("#viewer").css("display", "block");
        },
        failure: function (xhr, status) {
            $("#viewer").css("display", "none");
            $("results").css("display", "none");
            $("#error").css("display", "flex");
        },
        error: function (xhr, status) {
            $("#viewer").css("display", "none");
            $("#results").css("display", "none");
            $("#error").css("display", "flex");
        }
    });
    return false;
}

function get_result_item(r_txt) {
    return "<li class=\"result_item\"><a href=\"" + r_txt +
           "\" onclick=\"return get_cached('" + r_txt + "');\">" + r_txt +
           "</a></li>";
}

function deserialize_response(response) {
    if (response.length == 0) {
        return "No results found!";
    }
    r = "<ul id=\"result-list\">";
    r_txts = response.split('\0');
    for (var i = 0; i < r_txts.length; i++) {
        r_txt = r_txts[i];
        if (r_txt != "") {
            r += get_result_item(r_txt);
        }
    }
    r += "</ul>";
    return r;
}

function setup_result() {
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
}

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
        setup_result();
        $.get({
            url: url_txt,
            data: {
                query: query_txt
            },
            success: function (response) {
                $("#error").css("display", "none");
                $("#viewer").css("display", "none");
                $("#results").html(deserialize_response(response));
                $("#results").css("display", "flex");
            },
            failure: function (xhr, status) {
                $("#results").css("display", "none");
                $("#viewer").css("display", "none");
                $("#error").css("display", "flex");
            },
            error: function (xhr, status) {
                $("#results").css("display", "none");
                $("#viewer").css("display", "none");
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


