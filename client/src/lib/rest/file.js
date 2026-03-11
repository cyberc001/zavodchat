import Rest from '$lib/rest.js';
import Util from '$lib/util';
import {PUBLIC_BASE_REST} from '$env/static/public';

export default class {
	static upload(file, _then, _catch){
		const file_ext_idx = file.name.lastIndexOf(".");
		const ext = file_ext_idx === -1 ? "" : file.name.substring(file_ext_idx + 1);
		Rest.post("Uploading file", "files/upload",
				Util.form_data_from_object({ext, file}),
				_then, _catch);
	}

	static get_attachment_url(att_content){
		if(att_content.length > 0 && att_content[0] === '/')
			return PUBLIC_BASE_REST + att_content;
	}
}
