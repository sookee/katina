# Requirments

* php5 (>=5.4)
* php5-mysql
* php-gettext

Optional:

* memcached
* php5-memcache

# Install

## Apache + mod-php

Enable `mod_rewrite`.
By default Webkatti works under `/webkatti/`. You can change this in `.htaccess` and `conf-host.php` `$urlPrefix`.

## Nginx + php-fpm

Example config in `webkatti.nginx.conf`.
Set `server_name` to your domain, `root` path and `fastcgi_param SCRIPT_FILENAME` path to Webkatti's `index.php`.
By default Webkatti works under `/webkatti/`. You can change this in `location` directives and `conf-host.php` `$urlPrefix`.
If you already have nginx setup, copy only `location` directives to your config.

## Settings

Set database and other settings in `conf-host.php`, or at page `/webkatti/settings`.
If you want to change settings through web page, file `conf-host.php` should be writable to http server's user (usually it is `www-data`).
Settings page is available at `localhost` without authentication, or after login at page `/webkatti/login`.
Default login: `admin`, password: `123`.
