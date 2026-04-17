local WorldObjectNodes = {}

WorldObjectNodes.addAttachmentNode = function(self, node)
	self.attachment_nodes = self.attachment_nodes or {}
	add(self.attachment_nodes, node)
	node.layer = self.layer
	if self.layer and self.layer.refreshEntityBuckets then
		self.layer:refreshEntityBuckets(self)
	end
	return node
end

WorldObjectNodes.removeAttachmentNode = function(self, node)
	if not self.attachment_nodes then
		return
	end
	del(self.attachment_nodes, node)
	if self.layer and self.layer.refreshEntityBuckets then
		self.layer:refreshEntityBuckets(self)
	end
end

WorldObjectNodes.updateAttachmentNodes = function(self)
	if not self.attachment_nodes then
		return
	end
	for i = 1, #self.attachment_nodes do
		self.attachment_nodes[i]:updateRecursive()
	end
end

WorldObjectNodes.drawAttachmentNodes = function(self)
	if not self.attachment_nodes then
		return
	end
	for i = 1, #self.attachment_nodes do
		self.attachment_nodes[i]:drawRecursive()
	end
end

WorldObjectNodes.destroyAttachmentNodes = function(self)
	if not self.attachment_nodes then
		return
	end
	for i = #self.attachment_nodes, 1, -1 do
		self.attachment_nodes[i]:destroy()
	end
	self.attachment_nodes = {}
end

return WorldObjectNodes
