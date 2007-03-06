/*
 * Copyright (C) 2006, GrammarSoft Aps
 * and the VISL project at the University of Southern Denmark.
 * All Rights Reserved.
 *
 * The contents of this file are subject to the GrammarSoft Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.grammarsoft.com/GSPL or
 * http://visl.sdu.dk/GSPL.txt
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 */
#include "stdafx.h"
#include "Grammar.h"

using namespace CG3;

Grammar::Grammar() {
	last_modified = 0;
	name = 0;
	lines = 0;
	curline = 0;
	delimiters = 0;
	vislcg_compat_mode = false;
	mapping_prefix = '@';
	srand((uint32_t)time(0));
}

Grammar::~Grammar() {
	if (name) {
		delete name;
	}
	
	preferred_targets.clear();
	
	sections.clear();
	
	stdext::hash_map<uint32_t, Set*>::iterator iter_set;
	for (iter_set = sets_by_contents.begin() ; iter_set != sets_by_contents.end() ; iter_set++) {
		if (iter_set->second) {
			delete iter_set->second;
		}
	}
	sets_by_contents.clear();
	sets_by_name.clear();

	std::map<uint32_t, Anchor*>::iterator iter_anc;
	for (iter_anc = anchors.begin() ; iter_anc != anchors.end() ; iter_anc++) {
		if (iter_anc->second) {
			delete iter_anc->second;
		}
	}
	anchors.clear();
	
	stdext::hash_map<uint32_t, CompositeTag*>::iterator iter_ctag;
	for (iter_ctag = tags.begin() ; iter_ctag != tags.end() ; iter_ctag++) {
		if (iter_ctag->second) {
			delete iter_ctag->second;
		}
	}
	tags.clear();

	std::vector<Rule*>::iterator iter_rules;
	for (iter_rules = rules.begin() ; iter_rules != rules.end() ; iter_rules++) {
		if (*iter_rules) {
			delete *iter_rules;
		}
	}
	rules.clear();

	for (iter_rules = before_sections.begin() ; iter_rules != before_sections.end() ; iter_rules++) {
		if (*iter_rules) {
			delete *iter_rules;
		}
	}
	before_sections.clear();

	for (iter_rules = after_sections.begin() ; iter_rules != after_sections.end() ; iter_rules++) {
		if (*iter_rules) {
			delete *iter_rules;
		}
	}
	after_sections.clear();

	set_alias.clear();
}

void Grammar::addPreferredTarget(UChar *to) {
	Tag *tag = new Tag();
	tag->parseTag(to);
	tag->rehash();
	addTag(tag);
	preferred_targets.push_back(tag->hash);
}
void Grammar::addSet(Set *to) {
	assert(to);
	uint32_t nhash = hash_sdbm_uchar(to->name, 0);
	uint32_t chash = to->rehash();
	if (sets_by_name.find(nhash) == sets_by_name.end()) {
		sets_by_name[nhash] = chash;
	}
	if (sets_by_contents.find(chash) == sets_by_contents.end()) {
		sets_by_contents[chash] = to;
	}
}
Set *Grammar::getSet(uint32_t which) {
	Set *retval = 0;
	if (sets_by_contents.find(which) != sets_by_contents.end()) {
		retval = sets_by_contents[which];
	}
	else if (sets_by_name.find(which) != sets_by_name.end()) {
		retval = getSet(sets_by_name[which]);
	}
	return retval;
}

Set *Grammar::allocateSet() {
	return new Set;
}
void Grammar::destroySet(Set *set) {
	delete set;
}

void Grammar::addCompositeTag(CompositeTag *tag) {
	if (tag && tag->tags.size()) {
		tag->rehash();
		if (tags.find(tag->hash) != tags.end()) {
			delete tags[tag->hash];
		}
		tags[tag->hash] = tag;
	} else {
		u_fprintf(ux_stderr, "Error: Attempted to add empty composite tag to grammar!\n");
	}
}
void Grammar::addCompositeTagToSet(Set *set, CompositeTag *tag) {
	if (tag && tag->tags.size()) {
		if (tag->tags.size() == 1) {
			Tag *rtag = single_tags[tag->tags.begin()->second];
			set->tags_map[rtag->hash] = rtag->hash;
			if (rtag->features & (F_REGEXP | F_CASE_INSENSITIVE | F_NEGATIVE | F_NUMERICAL)) {
				set->single_tags_special[rtag->hash] = rtag->hash;
			}
			else if (rtag->features & F_FAILFAST) {
				set->single_tags_failfast[rtag->hash] = rtag->hash;
			}
			else {
				set->single_tags[rtag->hash] = rtag->hash;
			}

			if (rtag->type & T_ANY) {
				set->match_any = true;
			}
		} else {
			addCompositeTag(tag);
			set->tags_map[tag->hash] = tag->hash;
			set->tags[tag->hash] = tag->hash;
		}
	} else {
		u_fprintf(ux_stderr, "Error: Attempted to add empty composite tag to grammar and set!\n");
	}
}
CompositeTag *Grammar::allocateCompositeTag() {
	return new CompositeTag;
}
void Grammar::destroyCompositeTag(CompositeTag *tag) {
	delete tag;
}

Rule *Grammar::allocateRule() {
	return new Rule;
}
void Grammar::addRule(Rule *rule, std::vector<Rule*> *where) {
	where->push_back(rule);
}
void Grammar::destroyRule(Rule *rule) {
	delete rule;
}

Tag *Grammar::allocateTag(const UChar *tag) {
	Tag *fresh = new Tag;
	fresh->parseTag(tag);
	return fresh;
}
void Grammar::addTag(Tag *simpletag) {
	if (simpletag && simpletag->tag) {
		simpletag->rehash();
		if (single_tags.find(simpletag->hash) != single_tags.end()) {
			if (u_strcmp(single_tags[simpletag->hash]->tag, simpletag->tag) != 0) {
				u_fprintf(ux_stderr, "Warning: Hash collision between %S and %S!\n", single_tags[simpletag->hash]->tag, simpletag->tag);
			}
			delete single_tags[simpletag->hash];
		}
		single_tags[simpletag->hash] = simpletag;
	} else {
		u_fprintf(ux_stderr, "Error: Attempted to add empty tag to grammar!\n");
	}
}
void Grammar::addTagToCompositeTag(Tag *simpletag, CompositeTag *tag) {
	if (simpletag && simpletag->tag) {
		addTag(simpletag);
		tag->addTag(simpletag->hash);
	} else {
		u_fprintf(ux_stderr, "Error: Attempted to add empty tag to grammar and composite tag!\n");
	}
}
void Grammar::destroyTag(Tag *tag) {
	delete tag;
}

void Grammar::addAnchor(const UChar *to, uint32_t line) {
	uint32_t ah = hash_sdbm_uchar(to, 0);
	if (anchors.find(ah) != anchors.end()) {
		u_fprintf(ux_stderr, "Warning: Anchor '%S' redefined on line %u!\n", to, curline);
		delete anchors[ah];
		anchors.erase(ah);
	}
	Anchor *anc = new Anchor;
	anc->setName(to);
	anc->line = line;
	anchors[ah] = anc;
}

void Grammar::addAnchor(const UChar *to) {
	addAnchor(to, (uint32_t)(rules.size()+1));
}

void Grammar::setName(const char *to) {
	name = new UChar[strlen(to)+1];
	u_uastrcpy(name, to);
}

void Grammar::setName(const UChar *to) {
	name = new UChar[u_strlen(to)+1];
	u_strcpy(name, to);
}

void Grammar::trim() {
	set_alias.clear();
	sets_by_name.clear();

	stdext::hash_map<uint32_t, Tag*>::iterator iter_tags;
	for (iter_tags = single_tags.begin() ; iter_tags != single_tags.end() ; iter_tags++) {
		if (!iter_tags->second->type && !iter_tags->second->features) {
			Tag *tag = iter_tags->second;
			tag->type &= ~T_TEXTUAL;
		}
	}
}

void Grammar::reindex() {
	stdext::hash_map<uint32_t, Tag*>::iterator iter_tags;
	for (iter_tags = single_tags.begin() ; iter_tags != single_tags.end() ; iter_tags++) {
		Tag *tag = iter_tags->second;
		if (tag->tag[0] == mapping_prefix) {
			tag->type |= T_MAPPING;
		}
		else {
			tag->type &= ~T_MAPPING;
		}
	}

	stdext::hash_map<uint32_t, Set*>::iterator iter_sets;
	for (iter_sets = sets_by_contents.begin() ; iter_sets != sets_by_contents.end() ; iter_sets++) {
		iter_sets->second->reindex(this);
	}
}
