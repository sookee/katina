/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/ */

function APrettyPrint()
{
    var codes = document.getElementsByTagName('code'),
        l = codes.length;
    if (l)
    {
        for (var i=0; i<l; i++)
        {
            var code = codes[i],
                pre = code.parentNode;

            if (pre.tagName && pre.tagName.toLowerCase() === 'pre')
            {
                pre.className = 'prettyprint linenums';
            }
            else
            {
                code.className = 'prettyprint';
            }
        }

        var head = document.getElementsByTagName('head')[0],
            script = document.createElement('script');
        script.src= 'https://google-code-prettify.googlecode.com/svn/loader/prettify.js';
        script.onload = function()
        {
            prettyPrint();
        };
        head.appendChild(script);
    }
}
