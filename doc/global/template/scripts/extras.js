var vOffset = 65;

$(function () {
    $('a[href*=#]:not([href=#])').on('click', function (e) {
        if (e.which == 2)
            return true;
        var target = $(this.hash.replace(/(\.)/g, "\\$1"));
        target = target.length ? target : $('[name=' + this.hash.slice(1) + ']');
        if (target.length) {
            setTimeout(function () {
                $('html, body').animate({scrollTop: target.offset().top - vOffset}, 50);}, 50);
        }
    });
});

$(window).load(function () {
    var h = window.location.hash;
    var re = /[^a-z0-9_\.\#\-]/i
    if (h.length > 1 && !re.test(h)) {
        setTimeout(function () {
            $(window).scrollTop($(h.replace(/(\.)/g, "\\$1")).offset().top - vOffset);
        }, 0);
    }
});
