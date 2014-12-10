var vOffset = 65;

$(function () {
    $('a[href*=#]:not([href=#])').on('click', function () {
        var target = $(this.hash);
        target = target.length ? target : $('[name=' + this.hash.slice(1) + ']');
        if (target.length) {
            setTimeout(function () {
                $('html, body').animate({scrollTop: target.offset().top - vOffset}, 50);}, 50);
            // return false;
        }
    });
});

$(window).load(function () {
    var h = window.location.hash;
    var re = /[^a-z0-9_\#\-]/i
    if (h.length > 1 && !re.test(h)) {
        setTimeout(function () {
            $(window).scrollTop($(h).offset().top - vOffset);
        }, 0);
    }
});
