function addEventHandler(elem, eventType, handler) {
	if (elem.addEventListener)
	   elem.addEventListener (eventType,handler,false);
	else if (elem.attachEvent)
	   elem.attachEvent ('on'+eventType,handler); 
}

function inject(params) {

	$(params.target).getTransform(
		params.xsl
		, 'rest-api.php' + params.params
		, {
			params: { id: params.id, prefix: params.prefix }
			, callback: function() {
				if(params.id && params.action)
					addEventHandler(document.getElementById(params.id), 'change', params.action);
				if(params.action)
					params.action();
			}
		}
	);				
}

