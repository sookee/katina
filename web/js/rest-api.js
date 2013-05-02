function addEventHandler(elem, eventType, handler) {
	if (elem.addEventListener)
	   elem.addEventListener (eventType,handler,false);
	else if (elem.attachEvent)
	   elem.attachEvent ('on'+eventType,handler); 
}

/*
 *
 *	inject({
 *		id: 'sel-base' // id attribute of returned element
 *		, xsl: 'xsl/rest-sel-base.xsl' // XSLT to produce element
 *		, target: '#div-bases' // XPATH of element to inject
 *		, action: injectPolls // callback function
 *		, prefix: 'xsl-rest-sel-base' // CSS stype prefix
 *		, params:'?func=get_bases' // REST API function
 *	});		
 *
 */
function injectOld(params) {

	$(params.target).getTransform(
		params.xsl
		, 'rest-api.php' + params.params
		, {
			params: { id: params.id ? params.id : '', prefix: params.prefix ? params.prefix : '' }
			, callback: function() {
				if(params.id && params.action)
					addEventHandler(document.getElementById(params.id), 'change', params.action);
				if(params.action)
					params.action();
			}
		}
	);				
}

function inject(page, xsl) {

	$(page.target).getTransform(
		xsl.xsl
		, 'rest-api.php' + page.params
		, {
			params: xsl.options
			, callback: function() {
				if(xsl.options.id && page.action)
					addEventHandler(document.getElementById(xsl.options.id), 'change', page.action);
				if(page.action)
					page.action();
			}
		}
	);				
}

