
function ABigImage(match, onopen)
{
    var t = this;

    this.i = null;
    this.d = 0;
    this.as = [];

    this._overlay = document.createElement('div');
    this._overlay.className = 'ABigImage-overlay';
    document.documentElement.appendChild(this._overlay);

    this._layout = document.createElement('div');
    this._layout.className = 'ABigImage-layout';
    document.documentElement.appendChild(this._layout);

    this._wrapper = document.createElement('div');
    this._wrapper.className = 'ABigImage-wrapper';
    this._layout.appendChild(this._wrapper);

    this._box = document.createElement('div');
    this._box.className = 'ABigImage-box';
    this._wrapper.appendChild(this._box);

    this._prev = document.createElement('div');
    this._prev.className = 'ABigImage-prev';
    this._box.appendChild(this._prev);

    this._prev_text = document.createElement('span');
    this._prev_text.className = 'ABigImage-prev-text';
    this._prev.appendChild(this._prev_text);

    this._body = document.createElement('div');
    this._body.className = 'ABigImage-body';
    this._box.appendChild(this._body);

    this._close = document.createElement('div');
    this._close.className = 'ABigImage-close';
    this._box.appendChild(this._close);

    this._close_text = document.createElement('span');
    this._close_text.className = 'ABigImage-close-text';
    this._close.appendChild(this._close_text);

    this._top = document.createElement('div');
    this._top.className = 'ABigImage-top';
    this._body.appendChild(this._top);

    this._img = document.createElement('img');
    this._img.className = 'ABigImage-img';
    this._body.appendChild(this._img);

    this._img_next = document.createElement('img');
    this._img_next.className = 'ABigImage-img-next';
    this._body.appendChild(this._img_next);

    this._img_prev = document.createElement('img');
    this._img_prev.className = 'ABigImage-img-prev';
    this._body.appendChild(this._img_prev);

    this._bottom = document.createElement('div');
    this._bottom.className = 'ABigImage-bottom';
    this._body.appendChild(this._bottom);

    this._close.onclick = function()
    {
        return t.close();
    };

    if (match)
    {
        this.match = match;
    }

    if (onopen)
    {
        this.onopen = onopen;
    }

    this.find();
}



ABigImage.prototype.find = function()
{
    this.as = [];
    var t = this,
        i = -1;

    var as = document.getElementsByTagName('a');
    for (var l = as.length, k = 0; k < l; k++)
    {
        var a = as[k];
        if (this.match(a))
        {
            a._photoI = ++i;
            this.as[i] = a;
            a.onclick = function()
            {
                t.open(this._photoI);
                return false;
            };
        }
    }
};



ABigImage.prototype.match = function(a)
{
    var href = a.getAttribute('href');
    return href && href.match(/\.(jpe?g|png|gif)$/i);
};



ABigImage.prototype.open = function(i)
{
    if (i < 0 || i > this.as.length - 1)
    {
        return;
    }

    var t = this;
    this.i = i;
    this._img.setAttribute('src', this.as[i].getAttribute('href'));

    if (this.d === this.as.length - 1)
    {
        this._img.onclick = function()
        {
            return t.close();
        };
    }
    else
    {
        this._img_next.setAttribute('src', this.as[this.nextI()].getAttribute('href'));
        this._img.onclick = function()
        {
            return t.next();
        };
    }

    if (this.d === 1 - this.as.length)
    {
        this._prev.onclick = function()
        {
            return t.close();
        };
    }
    else
    {
        this._img_prev.setAttribute('src', this.as[this.prevI()].getAttribute('href'));
        this._prev.onclick = function()
        {
            return t.prev();
        };
    }

    this._overlay.className = 'ABigImage-overlay ABigImage-fadein';
    this._layout.className = 'ABigImage-layout ABigImage-visible';
    this._layout.style.top = document.body.scrollTop + 'px';
    this._layout.style.height = window.innerHeight + 'px';

    this.onopen(this.as[i]);

    return false;
};



ABigImage.prototype.nextI = function()
{
    var i = this.i + 1;
    if (i === this.as.length)
    {
        i = 0;
    }
    return i;
};



ABigImage.prototype.prevI = function()
{
    var i = this.i - 1;
    if (i === -1)
    {
        i = this.as.length - 1;
    }
    return i;
};



ABigImage.prototype.next = function()
{
    ++this.d;
    return this.open(this.nextI());
};



ABigImage.prototype.prev = function()
{
    --this.d;
    return this.open(this.prevI());
};



ABigImage.prototype.close = function()
{
    this.d = 0;
    this._overlay.className = 'ABigImage-overlay';
    this._layout.className = 'ABigImage-layout';
    return false;
};



ABigImage.prototype.onopen = function(a)
{
    this.top(a.getAttribute('title'));
};



ABigImage.prototype.top = function(html)
{
    this._top.innerHTML = html;
};



ABigImage.prototype.bottom = function(html)
{
    this._bottom.innerHTML = html;
};
